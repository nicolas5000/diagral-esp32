#include "DiagralController.hpp"
#include "DiagralCommands.hpp"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <esp_adc/adc_oneshot.h>
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include <stdio.h>
#include <time.h>
#include <algorithm>
#include "esp_log.h"
#include <ios>
#include <sstream>
#include <map>
#include <list>
#include <iostream>
#include <iomanip>
#include <format>
#include <mutex>

static const char *TAG = "diagral-ctrl";

constexpr int DIAGRAL_UART_BAUDRATE = 115200; // UART baudrate
constexpr int DIAGRAL_UART_RXBUFF_SIZE = 256; // Buffer size for RX on UART

constexpr uint32_t UART_WAIT_FIRSTBYTE_TIME = 780 / portTICK_PERIOD_MS;    // 780 ms max time to wait to receive first byte
constexpr uint32_t UART_WAIT_COMPLETEFRAME_TIME = 20 / portTICK_PERIOD_MS; // 20 ms max time to wait to receive complete frame once first byte is received
constexpr uint32_t UART_WAIT_FOR_ACK = 250 / portTICK_PERIOD_MS;           // 250 ms max time to wait for ACK
constexpr uint32_t UART_WAIT_AFTER_ACK = 5 / portTICK_PERIOD_MS;           // 5 ms max time to wait after sending ACK
constexpr uint32_t UART_WAIT_BEFORE_SEND = 4 / portTICK_PERIOD_MS;         // 4 ms time to wait after setting TX signal and before sending frame
constexpr uint32_t UART_RX_CHECK_INTERVAL = 20;                            // Time between 2 'Rx' check.

constexpr TickType_t MUTEX_MAX_WAIT_TICKS = 30000 * portTICK_PERIOD_MS; // 30 seconds
constexpr TickType_t UPDATE_STATE_WAKEUP_INTERVAL_MS = 5000;            // 5 seconds

constexpr UBaseType_t RX_FRAME_PROCESSING_PRIORITY = tskIDLE_PRIORITY + 6; // priority higher than IDLE, to perform RX frame work
constexpr UBaseType_t DEVICE_STATE_UPDATE_PRIORITY = tskIDLE_PRIORITY + 4; // priority higher than IDLE but less than RX processing, to launch status update
constexpr UBaseType_t DEVICE_STATE_CALLBACK_PRIORITY = tskIDLE_PRIORITY;   // priority same as IDLE (low priority)
constexpr UBaseType_t LOG_CALLBACK_PRIORITY = tskIDLE_PRIORITY;            // priority same as IDLE (low priority)

constexpr int64_t STATE_UPDATE_MAX_TIME_US = 3600000000; // 1 hour to next status update if nothing happens before
constexpr int64_t STATE_UPDATE_NEXT_TRY_US = 60000000;   // 60 seconds to wait if Get State failed

constexpr TickType_t UPDATE_VOLTAGE_INTERVAL_MS = 300000; // 300 seconds = 5 minutes

constexpr size_t LOG_MESSAGE_MAXSIZE = 256;

#define DIAG_LOGE(a, ...)                                                 \
  do                                                                      \
  {                                                                       \
    DiagralLog(ESP_LOG_ERROR, std::format(a __VA_OPT__(, ) __VA_ARGS__)); \
  } while (0)
#define DIAG_LOGI(a, ...)                                                \
  do                                                                     \
  {                                                                      \
    DiagralLog(ESP_LOG_INFO, std::format(a __VA_OPT__(, ) __VA_ARGS__)); \
  } while (0)

namespace Diagral
{
  static uart_port_t sUartPort = uart_port_t::UART_NUM_0; // UART port to use to communicate with Diagral system
  static uint8_t sFrameCounter = 0;                       // Frame counter for frames sent to Diagral system
  static QueueHandle_t sLogQueue = NULL;                  // Contains logs (LogQueueItem) to be sent to log callback
  static QueueHandle_t sDeviceStateQueue = NULL;          // Contains device state (DiagralDeviceState items) to be sent to status callback
  static DiagralDeviceState sDeviceState;                 // Current device state
  static uint64_t sNextStateUpdateTimestamp;              // Next time Device state should be updated
  static LoggerCallback sLoggerCallback;                  // Callback to send logs to
  static UpdatedDeviceCallback sDeviceStatusCallback;     // Callback to send device status updates to
  static BatteryMonitorConfig sBatteryConfig;             // Structure to store battery config to use in the update thread

  struct LogQueueItem
  {
    char log[LOG_MESSAGE_MAXSIZE];
    uint32_t timestamp;
    esp_log_level_t log_level;
  };

  /// @brief Generate a log from given string and level, and send it to registered callback
  /// @param log_level Log level
  /// @param log Log string
  static void DiagralLog(esp_log_level_t log_level, std::string log)
  {
    LogQueueItem logItem;
    memset(&logItem, 0, sizeof(LogQueueItem));
    logItem.log_level = log_level;
    logItem.timestamp = esp_log_timestamp();
    size_t messageSize = log.length() < LOG_MESSAGE_MAXSIZE ? log.length() : LOG_MESSAGE_MAXSIZE - 1;
    memcpy(logItem.log, log.c_str(), messageSize);
    if (!xQueueSendToBack(sLogQueue, &logItem, 0))
    {
      ESP_LOGE(TAG, "DiagralLog can't add log to queue!");
    }
  }

  /// @brief Convert a buffer to a HEX string representation
  /// (0x12 0x34 0xab -> "1234AB")
  /// @param len length of the input buffer
  /// @param buffer Buffer containing the bytes
  /// @return The string containing the HEX representation of the input buffer
  static std::string buffToHexString(uint8_t len, const uint8_t buffer[])
  {
    std::ostringstream convert;
    for (int a = 0; a < len; a++)
    {
      convert << std::format("{:02X}", buffer[a]);
    }
    return convert.str();
  }

  /// @brief Convert an HEX string to a byte buffer
  /// @param hex HEX string (eg "1234AB")
  /// @param buffer Buffer that will receive the string conversion (eg, will be 0x12 0x34 0xab)
  /// @param len Length of the allocated buffer, needs to be at least HEX string length / 2
  static void HexStringToBuff(const std::string &hex, uint8_t buffer[], uint8_t len)
  {
    if (len < hex.length() / 2)
    {
      DIAG_LOGE("HexStringToBuff: buffer is too short! ({}, need {})", len, hex.length() / 2);
      return;
    }
    std::istringstream ss(hex);
    std::string s2;
    unsigned int i = 0;
    while ((ss >> std::setw(2) >> s2))
    {
      buffer[i / 2] = (uint8_t)strtol(s2.c_str(), nullptr, 16);
      i += 2;
    }
  }

  /// @brief Low priority task that reads battery voltage
  /// @param arg not used
  static void read_battery_voltage_task(void *arg)
  {
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = sBatteryConfig.adc_unit;
    init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    adc_cali_curve_fitting_config_t cali_config = {};
    cali_config.unit_id = init_config.unit_id;
    cali_config.chan = sBatteryConfig.adc_channel;
    cali_config.atten = ADC_ATTEN_DB_12;
    cali_config.bitwidth = ADC_BITWIDTH_12;
    adc_cali_handle_t cali_handle = NULL;
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,    // ~3.3V full-scale voltage
        .bitwidth = ADC_BITWIDTH_12, // 12-bit resolution (0-4095)
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, sBatteryConfig.adc_channel, &config));
    int adc_value, voltage;
    for (;;)
    {
      ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, sBatteryConfig.adc_channel, &adc_value));
      ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_value, &voltage));
      if (voltage < sBatteryConfig.min_voltage_mv)
        sDeviceState.battery = 0;
      else if (voltage > sBatteryConfig.max_voltage_mv)
        sDeviceState.battery = 100;
      else
        sDeviceState.battery = (voltage - sBatteryConfig.min_voltage_mv) * 100 / (sBatteryConfig.max_voltage_mv - sBatteryConfig.min_voltage_mv);
      ESP_LOGI(TAG, "Battery - ADC Value %d, voltage %.2f -> %d%", adc_value, voltage / 1000.0, sDeviceState.battery);
      if (!xQueueSendToBack(sDeviceStateQueue, &sDeviceState, 0))
      {
        DIAG_LOGE("read_battery_voltage_task: can't add device state to queue!");
      }
      vTaskDelay(pdMS_TO_TICKS(UPDATE_VOLTAGE_INTERVAL_MS)); // Wait until next loop to check again
    }
  }

  /// @brief Low priority task that takes logs from queue and send them to registered callback
  /// @param arg Pointer to DiagralController object
  static void process_log_task(void *arg)
  {
    DiagralController *diagralController = static_cast<DiagralController *>(arg);
    for (;;)
    {
      LogQueueItem logItem;
      if (xQueueReceive(sLogQueue, &logItem, portMAX_DELAY))
      {
        if (diagralController->isVerbose() && sLoggerCallback)
        {
          sLoggerCallback(logItem.log_level, TAG, std::format("({}) {}", logItem.timestamp, logItem.log));
        }
      }
    }
  }

  /// @brief Low priority task that takes device states from queue and send them to registered callback
  /// @param arg currently not used
  static void process_devicestate_task(void *arg)
  {
    for (;;)
    {
      DiagralDeviceState deviceState;
      if (xQueueReceive(sDeviceStateQueue, &deviceState, portMAX_DELAY))
      {
        if (sDeviceStatusCallback) // we have a callback
        {
          sDeviceStatusCallback(deviceState);
        }
      }
    }
  }

  /// @brief Task processing received frames
  /// @param arg Pointer to DiagralController object
  static void process_rxframe_task(void *arg)
  {
    DiagralController *controller = static_cast<DiagralController *>(arg);
    controller->ProcessReceivedFrameTask();
  }

  /// @brief Task responsible for updating device state when needed
  /// @param arg Pointer to DiagralController object
  static void update_device_state_task(void *arg)
  {
    DiagralController *controller = static_cast<DiagralController *>(arg);
    controller->UpdateDeviceStateTask();
  }

  DiagralController::DiagralController(LoggerCallback logger, UpdatedDeviceCallback deviceStatusCallback)
      : mInitialized(false),
        mVerbose(true)
  {
    sLoggerCallback = logger;
    sDeviceStatusCallback = deviceStatusCallback;

    sNextStateUpdateTimestamp = 20000000; // force update 20s after boot

    // Create logger queue and task
    sLogQueue = xQueueCreate(30, sizeof(LogQueueItem));
    xTaskCreate(process_log_task, "process_log_task", 4096, this, LOG_CALLBACK_PRIORITY, NULL);

    // Create Device state queue and task
    sDeviceStateQueue = xQueueCreate(20, sizeof(DiagralDeviceState));
    xTaskCreate(process_devicestate_task, "process_devicestate_task", 4096, NULL, DEVICE_STATE_CALLBACK_PRIORITY, NULL);

    // start rxframe task
    xTaskCreate(process_rxframe_task, "process_rxframe_task", 8192, this, RX_FRAME_PROCESSING_PRIORITY, NULL);
    // start status update task
    xTaskCreate(update_device_state_task, "update_devices_state_task", 4096, this, DEVICE_STATE_UPDATE_PRIORITY, NULL);
  }

  bool DiagralController::Init(int port, int tx, int rx, int signal, bool isPassive)
  {
    mPassiveMode = isPassive;
    sUartPort = (uart_port_t)port;
    mTxSignalPin = signal;

    // Initialize UART
    uart_config_t uart_config = {};
    uart_config.baud_rate = DIAGRAL_UART_BAUDRATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(sUartPort, DIAGRAL_UART_RXBUFF_SIZE, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(sUartPort, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(sUartPort, tx, rx, GPIO_NUM_NC, GPIO_NUM_NC));

    // Initialize TX signal pin to Input Pulldown
    esp_err_t err = ConfigureTxSignalPin(false);
    if (err != ESP_OK)
    {
      DIAG_LOGE("gpio_config failed : %s", esp_err_to_name(err));
      return false;
    }

    mInitialized = true;

    return true;
  }

  void DiagralController::MonitorBattery(BatteryMonitorConfig config)
  {
    memcpy(&sBatteryConfig, &config, sizeof(BatteryMonitorConfig));
    // start battery voltage monitoring task
    xTaskCreate(read_battery_voltage_task, "read_battery_voltage_task", 4096, NULL, tskIDLE_PRIORITY, NULL);
  }

  void DiagralController::ProcessReceivedFrameTask()
  {
    while (!mInitialized)
      vTaskDelay(pdMS_TO_TICKS(UART_RX_CHECK_INTERVAL));
    DiagralFrame frame;
    while (true)
    {
      if (gpio_get_level(static_cast<gpio_num_t>(mTxSignalPin)) == 0)
      {
        // Diagral system is not sending: let commands to be sent between 2 'Rx' checks
        vTaskDelay(pdMS_TO_TICKS(UART_RX_CHECK_INTERVAL));
      }
      if (mUartMutex.try_lock())
      {
        // We have the mutex, it means we are not sending something: we have to check if there is incoming frame to process!
        if (ReceiveFrame(frame))
        {
          if (mPassiveMode)
          {
            continue; // don't reply / don't process command
          }
          // Process received frame
          switch (frame.data[0]) // switch on I = xx
          {
          case CMD_BINARY_MANAGEMENT:
            if (frame.data[1] != 0x20)
            {
              DIAG_LOGE("ProcessReceivedFrameTask - Invalid data[1], expected 0x20, got 0x{:02X}", frame.data[1]);
              break;
            }
            switch (frame.data[2]) // switch on J = xx for I = 0x04
            {
            case SUBCMD_BINARY_DESCRIPTION_COMMAND: // Diagral system will send a binary record
              // Just accept it by sending max fragment size
              {
                DiagralFrame response;
                if (!create_binary_max_fragment_response(response) || !TransmitFrame(response))
                {
                  DIAG_LOGE("ProcessReceivedFrameTask failed to send I=04/J=02 response!");
                }
                break;
              }
            case SUBCMD_BINARY_FRAGMENT: // Diagral system sends a binary fragment, just accept it!
            {
              DiagralFrame response;
              if (frame.data_length < 4)
              {
                DIAG_LOGE("ProcessReceivedFrameTask I=04/J=03 invalid min size!");
                break;
              }
              uint16_t fragmentIndex = (frame.data[3] << 8) + frame.data[4];
              if (!create_binary_fragment_ack(response, fragmentIndex) || !TransmitFrame(response))
              {
                DIAG_LOGE("ProcessReceivedFrameTask failed to send I=04/J=09 response!");
              }
              break;
            }
            case SUBCMD_BINARY_READ: // Diagral system asks for an binary record to send back
            {
              DiagralFrame response;
              if (frame.data_length < 9)
              {
                DIAG_LOGE("ProcessReceivedFrameTask I=04/J=08 invalid min size!");
                break;
              }
              // First send binary description
              if (!create_binary_response_for_read_binary(response, frame) || !TransmitFrame(response))
              {
                DIAG_LOGE("ProcessReceivedFrameTask failed to send I=04/J=01 response!");
              }
              // Then read max fragment size (but don't use...)
              if (!ReceiveFrame(frame) || frame.data_length != 5 || frame.data[0] != CMD_BINARY_MANAGEMENT || frame.data[2] != SUBCMD_BINARY_MAX_FRAG_RESPONSE)
              {
                DIAG_LOGE("ProcessReceivedFrameTask received invalid I=04/J=02 response!");
                break;
              }
              // Then send hardcoded binary value
              if (!create_binary_fragment_request(response) || !SendAndReceive(response, frame) || frame.data_length != 5 || frame.data[0] != CMD_BINARY_MANAGEMENT || frame.data[2] != SUBCMD_BINARY_FRAGMENT_ACK)
              {
                DIAG_LOGE("ProcessReceivedFrameTask received invalid I=04/J=09 response!");
                break;
              }
              // Then finalize binary transfer
              if (!create_binary_end_request(response) || !TransmitFrame(response))
              {
                DIAG_LOGE("ProcessReceivedFrameTask failed to send I=04/J=04 response!");
              }
              break;
            }
            case SUBCMD_BINARY_END: // This frame doesn't require response, so ignore it as we don't manage binary records.
              break;
            default:
              DIAG_LOGE("ProcessReceivedFrameTask - Unexpected J=0x{:02X} for I=0x{:02X}", frame.data[2], frame.data[0]);
              break;
            }
            break;
          case CMD_GENERIC_MANAGEMENT:
            switch (frame.data[1]) // switch on J = xx for I = 0x07
            {
            case SUBCMD_GEN_IDLE_SYSTEM:
              // Don't know what this frame is and it doesn't require response, so ignore it.
              break;
            case SUBCMD_GEN_STATE_NOTIFICATION:
            case SUBCMD_GEN_ALERT_NOTIFICATION:
            case SUBCMD_GEN_DETECTION_NOTIFICATION:
            case SUBCMD_GEN_TAMPERING_NOTIFICATION:
              UpdateDeviceState(frame);
              break;
            case SUBCMD_GEN_PIN_MANAGEMENT:
              switch (frame.data[2]) // switch on K = xx for J = 0x71
              {
              case SUBCMD_PIN_CHANGE_NOTIFICATION:
                DIAG_LOGI("Received PIN code change notification");
                break;
              default:
                DIAG_LOGE("ProcessReceivedFrameTask - Unmanaged K=0x{:02X} for J=0x{:02X} and I=0x{:02X}", frame.data[2], frame.data[1], frame.data[0]);
                break;
              }
              break;
            case SUBCMD_GEN_POWER_ON_STATE:
              // Don't know what this frame is but it requires a response, so send response.
              {
                DiagralFrame response;
                if (!create_J96_response(response) || !TransmitFrame(response))
                {
                  DIAG_LOGE("ProcessReceivedFrameTask failed to send K41 response!");
                }
                break;
              }
            case SUBCMD_GEN_OTHERS:
              switch (frame.data[2]) // switch on K = xx for J = 0xB0
              {
              case SUBCMD_OTHERS_LANGUAGE:
                sDeviceState.language = (DiagralLanguage)frame.data[4];
                if (!xQueueSendToBack(sDeviceStateQueue, &sDeviceState, 0))
                {
                  DIAG_LOGE("ProcessReceivedFrameTask can't add device to queue!");
                }
                break;
              case SUBCMD_OTHERS_SYSTEM_31:
                // Don't know what this frame is and it doesn't require response, so ignore it.
                break;
              case SUBCMD_OTHERS_SYSTEM_40:
                // Don't know what this frame is but it requires a response, so send response.
                {
                  DiagralFrame response;
                  if (!create_K41_response(response) || !TransmitFrame(response))
                  {
                    DIAG_LOGE("ProcessReceivedFrameTask failed to send K41 response!");
                  }
                  break;
                }
              case SUBCMD_OTHERS_POWER_NOTIFICATION:
                sDeviceState.power = (DiagralPowerSupply)frame.data[3];
                if (!xQueueSendToBack(sDeviceStateQueue, &sDeviceState, 0))
                {
                  DIAG_LOGE("ProcessReceivedFrameTask can't add device to queue!");
                }
                break;
              case SUBCMD_OTHERS_STATE_REJECTED:
                DIAG_LOGE("System rejected state change!");
                break;
              case SUBCMD_OTHERS_SETTING_CHECK_EXIST:
              case SUBCMD_OTHERS_SETTING_GET:
              case SUBCMD_OTHERS_SETTING_CHECK_VALUE:
              case SUBCMD_OTHERS_SETTING_SET:
              case SUBCMD_OTHERS_SETTING_DEL:
              case SUBCMD_OTHERS_MISC:
              default:
                DIAG_LOGE("ProcessReceivedFrameTask - Unmanaged K=0x{:02X} for J=0x{:02X} and I=0x{:02X}", frame.data[2], frame.data[1], frame.data[0]);
                break;
              }
              break;
            default:
              DIAG_LOGE("ProcessReceivedFrameTask - Unexpected J=0x{:02X} for I=0x{:02X}", frame.data[1], frame.data[0]);
              break;
            }
            break;
          default:
            DIAG_LOGE("ProcessReceivedFrameTask - Invalid I=0x{:02X}", frame.data[0]);
            break;
          }
        }
        // Finally, release mutex
        mUartMutex.unlock();
      }
      else
      {
        // Currently sending, wait a few time
        vTaskDelay(pdMS_TO_TICKS(UART_RX_CHECK_INTERVAL));
      }
    }
  }

  void DiagralController::UpdateDeviceStateTask()
  {
    for (;;) // infinite loop
    {
      if (!isPassive() && mInitialized) // not passive, check if we should update device state!
      {
        if ((esp_timer_get_time() > sDeviceState.lastStateTimestamp + STATE_UPDATE_MAX_TIME_US || sDeviceState.lastStateTimestamp == 0) // previous update is a long time ago
            && (esp_timer_get_time() > sNextStateUpdateTimestamp))                                                                      // and previous attempt is expired
        {
          // So let's update device status
          mUartMutex.lock();
          DiagralFrame request;
          DiagralFrame response;
          if (!create_get_set_state_command(request, DIAGRAL_DATA_STATE_GET) || !SendAndReceive(request, response))
          {
            DIAG_LOGE("UpdateDeviceStateTask: failed to send request or get response!");
            sNextStateUpdateTimestamp = esp_timer_get_time() + STATE_UPDATE_NEXT_TRY_US; // try again later
          }
          else if (response.data_length != 4 || response.data[0] != CMD_GENERIC_MANAGEMENT || response.data[1] != SUBCMD_GEN_OTHERS || response.data[2] != SUBCMD_OTHERS_RESPONSE || response.data[3] != DIAGRAL_DATA_ACK_NO_ERROR)
          {
            if ((response.data_length >= 4) && (response.data[0] == CMD_GENERIC_MANAGEMENT) && response.data[1] == SUBCMD_GEN_STATE_NOTIFICATION)
            {
              UpdateDeviceState(response);
            }
            else
              DIAG_LOGE("UpdateDeviceStateTask: invalid response!");
          }
          mUartMutex.unlock();
        }
      }
      vTaskDelay(pdMS_TO_TICKS(UPDATE_STATE_WAKEUP_INTERVAL_MS)); // Wait until next loop to check again
    }
  }

  bool DiagralController::ForceDeviceStateUpdate()
  {
    if (!mInitialized || mPassiveMode)
    {
      DIAG_LOGE("ForceDeviceStatusUpdate: invalid state! (not initialized or not listening or passive mode)");
      return false;
    }
    sDeviceState.lastStateTimestamp = 0;
    sNextStateUpdateTimestamp = 0;
    return true;
  }

  bool DiagralController::SendRaw(const std::string &rawFrame)
  {
    if (!mInitialized || mPassiveMode)
    {
      DIAG_LOGE("SendRaw: invalid state! (not initialized or passive mode)");
      return false;
    }
    if ((rawFrame.length() < 2) || (rawFrame.length() > 2 * FRAME_DATA_MAX_SIZE))
    {
      DIAG_LOGE("SendRaw: invalid frame to send, must be between 1 and %d bytes!", FRAME_DATA_MAX_SIZE);
      return false;
    }
    // Create request
    DiagralFrame request;
    HexStringToBuff(rawFrame, request.data, FRAME_DATA_MAX_SIZE);
    request.data_length = rawFrame.length() / 2;
    // Take mutex and change task priority
    mUartMutex.lock();
    UBaseType_t currentPriority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, RX_FRAME_PROCESSING_PRIORITY);
    // Send request
    bool ret = TransmitFrame(request);
    // Release mutex and restore task priority
    vTaskPrioritySet(NULL, currentPriority);
    mUartMutex.unlock();
    return ret;
  }

  bool DiagralController::CheckPinCode(std::string pinCode)
  {
    if (!mInitialized || mPassiveMode)
    {
      DIAG_LOGE("CheckPinCode: invalid state! (not initialized or passive mode)");
      return false;
    }
    DiagralFrame request, response;
    bool ret = true;
    // Take mutex and change task priority
    mUartMutex.lock();
    UBaseType_t currentPriority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, RX_FRAME_PROCESSING_PRIORITY);
    // Create and send request to get PIN length, check response
    if (!create_pin_get_length_command(request) || !SendAndReceive(request, response))
    {
      DIAG_LOGE("CheckPinCode: failed to send pin_get_length request or get response!");
      ret = false;
    }
    else if (response.data_length != 4 || response.data[0] != CMD_GENERIC_MANAGEMENT || response.data[1] != SUBCMD_GEN_PIN_MANAGEMENT || response.data[2] != SUBCMD_PIN_GET_LENGTH_RESPONSE)
    {
      DIAG_LOGE("CheckPinCode: invalid response!");
      ret = false;
    }
    else if (response.data[3] != pinCode.length())
    {
      DIAG_LOGE("CheckPinCode: invalid PIN code!");
      ret = false;
    }
    if (ret)
    {
      // PIN length is correct, check PIN value!
      if (!create_pin_check_command(request, pinCode) || !SendAndReceive(request, response))
      {
        DIAG_LOGE("CheckPinCode: failed to send pin_check request or get response!");
        ret = false;
      }
      else if (response.data_length != 4 || response.data[0] != CMD_GENERIC_MANAGEMENT || response.data[1] != SUBCMD_GEN_PIN_MANAGEMENT || response.data[2] != SUBCMD_PIN_CHECK_RESPONSE)
      {
        DIAG_LOGE("CheckPinCode: invalid response!");
        ret = false;
      }
      else if (response.data[3] != 0x00)
      {
        DIAG_LOGE("CheckPinCode: invalid PIN code!");
        ret = false;
      }
    }
    // Release mutex and restore task priority
    vTaskPrioritySet(NULL, currentPriority);
    mUartMutex.unlock();
    return ret;
  }

  bool DiagralController::ArmAway(uint8_t zones)
  {
    if (sDeviceState.mode != DiagralMode::DIAGRAL_MODE_IDLE)
    {
      DIAG_LOGE("ArmAway - Invalid state {}", DiagralModeToString(sDeviceState.mode));
      return false;
    }
    if (zones != DIAGRAL_DATA_ZONES_ALL)
      return SetGetState(DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL, 0, zones);
    else
      return SetGetState(DIAGRAL_DATA_STATE_SET_ARM_ALL);
  }

  bool DiagralController::ArmHome()
  {
    if (sDeviceState.mode != DiagralMode::DIAGRAL_MODE_IDLE)
    {
      DIAG_LOGE("ArmHome - Invalid state {}", DiagralModeToString(sDeviceState.mode));
      return false;
    }
    return SetGetState(DIAGRAL_DATA_STATE_SET_ARM_HOME);
  }

  bool DiagralController::Disarm(uint8_t zones)
  {
    if (sDeviceState.mode != DiagralMode::DIAGRAL_MODE_IDLE)
    {
      DIAG_LOGE("Disarm - Invalid state {}", DiagralModeToString(sDeviceState.mode));
      return false;
    }
    if (zones != DIAGRAL_DATA_ZONES_ALL)
      return SetGetState(DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL, zones);
    else
      return SetGetState(DIAGRAL_DATA_STATE_SET_DISARM_ALL);
  }

  esp_err_t DiagralController::ConfigureTxSignalPin(bool txMode)
  {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << mTxSignalPin);
    cfg.mode = txMode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    esp_err_t err;
    err = gpio_config(&cfg);
    if (err == ESP_OK && txMode)
    {
      err = gpio_set_level(static_cast<gpio_num_t>(mTxSignalPin), 1);
    }
    return err;
  }

  uint8_t DiagralController::ComputeCRC(uint8_t buffer[], uint8_t length)
  {
    uint8_t crc = 0x00;
    for (uint8_t it = 0; it < length; it++)
    {
      crc ^= buffer[it]; // XOR
    }
    return crc;
  }

  bool DiagralController::TransmitFrame(const DiagralFrame &diagralFrame)
  {
    if (!mInitialized || mPassiveMode)
      return false;
    if (diagralFrame.data_length > FRAME_DATA_MAX_SIZE)
    {
      DIAG_LOGE("TransmitFrame: frame length too long! {}", diagralFrame.data_length);
      return false;
    }
    uint8_t data[FRAME_MAX_SIZE];
    data[0] = diagralFrame.data_length + 2; // Length = DATA length + counter + CRC
    data[1] = sFrameCounter++;              // frame counter, incremented at each frame sent to Diagral system
    memcpy(data + 2, diagralFrame.data, diagralFrame.data_length);
    // Compute CRC
    data[diagralFrame.data_length + 2] = ComputeCRC(data, diagralFrame.data_length + 2);
    // Set TX signal at high state
    ConfigureTxSignalPin(true);
    uart_flush_input(sUartPort);
    vTaskDelay(pdMS_TO_TICKS(UART_WAIT_BEFORE_SEND));
    // Send length, frame and CRC
    uart_write_bytes(sUartPort, data, diagralFrame.data_length + 3);
    if (mVerbose)
    {
      DIAG_LOGI("TransmitFrame: Sent at {}, {}", esp_timer_get_time(), buffToHexString(diagralFrame.data_length + 3, data));
    }
    // Read ACK
    int readLen = uart_read_bytes(sUartPort, data, FRAME_ACK_SIZE, UART_WAIT_FOR_ACK);
    // Check ACK
    if (readLen != FRAME_ACK_SIZE)
    {
      DIAG_LOGE("TransmitFrame: ACK frame invalid length! {}", readLen);
      // Release TX signal
      ConfigureTxSignalPin(false);
      return false;
    }
    else if (data[0] == 0x06)
    {
      // Due to ESP32 reset, counter is reset so Diagral sends a I=0x07, J=0xB0 K=0x28 response instead of a ACK. We must send ACK back!
      readLen += uart_read_bytes(sUartPort, data + readLen, 2, UART_WAIT_FOR_ACK);
      if (mVerbose)
      {
        DIAG_LOGI("TransmitFrame: Received at {}, {}", esp_timer_get_time(), buffToHexString(readLen, data));
      }
      if ((readLen == 7) && (data[2] == CMD_GENERIC_MANAGEMENT) && (data[3] == SUBCMD_GEN_OTHERS) && (data[4] == SUBCMD_OTHERS_RESPONSE) && (data[5] == DIAGRAL_DATA_ACK_NO_ERROR))
      {
        // Check CRC
        uint8_t crc = ComputeCRC(data, data[0]);
        if (crc != data[data[0]])
        {
          DIAG_LOGE("ReceiveFrame: CRC error! (expected {}, received {})", crc, data[data[0]]);
          // Release TX signal
          ConfigureTxSignalPin(false);
          return false;
        }
        // Send ACK
        if (!mPassiveMode)
        {
          uint8_t ack[5] = {0x04, data[1], 0x40, 0x01, 0xFF};
          ack[4] = ComputeCRC(ack, sizeof(ack) - 1);
          uart_write_bytes(sUartPort, (const char *)ack, sizeof(ack));
          vTaskDelay(pdMS_TO_TICKS(UART_WAIT_AFTER_ACK));
          // Release TX signal
          ConfigureTxSignalPin(false);
          return true;
        }
      }
    }
    if (mVerbose)
    {
      DIAG_LOGI("TransmitFrame: Received at {}, {}", esp_timer_get_time(), buffToHexString(readLen, data));
    }
    // Release TX signal
    ConfigureTxSignalPin(false);
    if ((data[0] != FRAME_ACK_SIZE - 1) || (data[1] != sFrameCounter - 1) || (data[2] != CMD_ACK) || (data[3] != DIAGRAL_DATA_ACK_NO_ERROR) || (data[4] != ComputeCRC(data, FRAME_ACK_SIZE - 1)))
    {
      DIAG_LOGE("TransmitFrame: invalid ACK frame or error!");
      return false;
    }
    return true;
  }

  bool DiagralController::ReceiveFrame(DiagralFrame &frame)
  {
    if (!mInitialized)
      return false;
    // Read RX frame
    uint8_t frameLength = 0;
    uint8_t data[FRAME_MAX_SIZE];

    int readLen = uart_read_bytes(sUartPort, &frameLength, 1, UART_WAIT_FIRSTBYTE_TIME);
    if (readLen < 1 || frameLength < 1)
    {
      return false;
    }
    if (readLen > FRAME_DATA_MAX_SIZE)
    {
      DIAG_LOGE("ReceiveFrame: frame length too long! {})", frameLength);
      return false;
    }
    data[0] = frameLength;
    readLen = uart_read_bytes(sUartPort, data + 1, frameLength, UART_WAIT_COMPLETEFRAME_TIME);
    if (readLen != frameLength)
    {
      DIAG_LOGE("ReceiveFrame: didn't read complete frame! (expected {}, read {})", frameLength, readLen);
      return false;
    }
    if (mVerbose)
    {
      DIAG_LOGI("ReceiveFrame: Received at {}, {}", esp_timer_get_time(), buffToHexString(frameLength + 1, data));
    }
    // Check CRC
    uint8_t crc = ComputeCRC(data, frameLength);
    if (crc != data[frameLength])
    {
      DIAG_LOGE("ReceiveFrame: CRC error! (expected {}, received {})", crc, data[frameLength]);
      return false;
    }
    // Send ACK
    if (!mPassiveMode)
    {
      uint8_t ack[5] = {0x04, data[1], 0x40, 0x01, 0xFF};
      ack[4] = ComputeCRC(ack, sizeof(ack) - 1);
      uart_write_bytes(sUartPort, (const char *)ack, sizeof(ack));
      vTaskDelay(pdMS_TO_TICKS(UART_WAIT_AFTER_ACK));
    }
    // Copy frame data to DiagralFrame (without length, counter and CRC)
    frame.data_length = frameLength - 2; // without Length byte, counter byte and CRC byte
    memcpy(frame.data, data + 2, frameLength - 2);
    return true;
  }

  bool DiagralController::SendAndReceive(const DiagralFrame &txFrame, DiagralFrame &rxFrame)
  {
    bool ret = TransmitFrame(txFrame);
    if (ret)
      ret = ReceiveFrame(rxFrame);
    return ret;
  }

  bool DiagralController::SetGetState(uint8_t command, uint8_t disarmZones, uint8_t armZones)
  {
    if (!mInitialized || mPassiveMode)
    {
      DIAG_LOGE("SetGetState: invalid state! (not initialized or passive mode)");
      return false;
    }
    DiagralFrame request, response;
    bool ret = true;
    // Take mutex and change task priority
    mUartMutex.lock();
    UBaseType_t currentPriority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, RX_FRAME_PROCESSING_PRIORITY);
    // Create and send request, check response
    if (!create_get_set_state_command(request, command, disarmZones, armZones) || !SendAndReceive(request, response))
    {
      DIAG_LOGE("SetGetState: failed to send request or get response!");
      ret = false;
    }
    else if (response.data_length != 4 || response.data[0] != CMD_GENERIC_MANAGEMENT || response.data[1] != SUBCMD_GEN_OTHERS || response.data[2] != SUBCMD_OTHERS_RESPONSE || response.data[3] != DIAGRAL_DATA_ACK_NO_ERROR)
    {
      DIAG_LOGE("SetGetState: invalid response!");
      ret = false;
    }
    // Release mutex and restore task priority
    vTaskPrioritySet(NULL, currentPriority);
    mUartMutex.unlock();
    return ret;
  }

  void DiagralController::UpdateDeviceState(const DiagralFrame &statusFrame)
  {
    if (statusFrame.data_length < 2 || statusFrame.data[0] != CMD_GENERIC_MANAGEMENT)
    {
      DIAG_LOGE("UpdateDeviceState: invalid frame!");
      return;
    }
    switch (statusFrame.data[1]) // switch on J = xx for I = 0x07
    {
    case SUBCMD_GEN_STATE_NOTIFICATION:
      if (statusFrame.data_length < 16)
      {
        DIAG_LOGE("UpdateDeviceState: invalid frame length for J=0x{:02X}!", statusFrame.data[1]);
        return;
      }
      sDeviceState.mode = (DiagralMode)statusFrame.data[2];
      // Update zones state
      if (statusFrame.data[3] & DIAGRAL_DATA_STATE_ARM_HOME_BIT)
      {
        // Fill corresponding zones state with DIAGRAL_STATE_ARMED_HOME
        if (statusFrame.data[8] & DIAGRAL_DATA_ZONE1)
          sDeviceState.zone1 = DIAGRAL_STATE_ARMED_HOME;
        if (statusFrame.data[8] & DIAGRAL_DATA_ZONE2)
          sDeviceState.zone2 = DIAGRAL_STATE_ARMED_HOME;
        if (statusFrame.data[8] & DIAGRAL_DATA_ZONE3)
          sDeviceState.zone3 = DIAGRAL_STATE_ARMED_HOME;
        if (statusFrame.data[8] & DIAGRAL_DATA_ZONE4)
          sDeviceState.zone4 = DIAGRAL_STATE_ARMED_HOME;
      }
      else if (statusFrame.data[3] & (DIAGRAL_DATA_STATE_ARM_PARTIAL_BIT | DIAGRAL_DATA_STATE_ARM_ALL_BIT))
      {
        if (statusFrame.data[3] & DIAGRAL_DATA_STATE_ARMING_BIT)
        {
          // Fill corresponding zones state with DIAGRAL_STATE_ARMING
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE1)
            sDeviceState.zone1 = DIAGRAL_STATE_ARMING;
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE2)
            sDeviceState.zone2 = DIAGRAL_STATE_ARMING;
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE3)
            sDeviceState.zone3 = DIAGRAL_STATE_ARMING;
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE4)
            sDeviceState.zone4 = DIAGRAL_STATE_ARMING;
        }
        else
        {
          // Fill corresponding zones state with DIAGRAL_STATE_ARMED
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE1)
            sDeviceState.zone1 = DIAGRAL_STATE_ARMED;
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE2)
            sDeviceState.zone2 = DIAGRAL_STATE_ARMED;
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE3)
            sDeviceState.zone3 = DIAGRAL_STATE_ARMED;
          if (statusFrame.data[8] & DIAGRAL_DATA_ZONE4)
            sDeviceState.zone4 = DIAGRAL_STATE_ARMED;
        }
      }
      else if ((statusFrame.data[3] & DIAGRAL_DATA_STATE_RESPONSE_TO_REQUEST_BIT) == 0x00)
      {
        // Not from a request so it is a disarm state! Fill corresponding zones state with DIAGRAL_STATE_DISARMED
        if (statusFrame.data[6] & DIAGRAL_DATA_ZONE1)
          sDeviceState.zone1 = DIAGRAL_STATE_DISARMED;
        if (statusFrame.data[6] & DIAGRAL_DATA_ZONE2)
          sDeviceState.zone2 = DIAGRAL_STATE_DISARMED;
        if (statusFrame.data[6] & DIAGRAL_DATA_ZONE3)
          sDeviceState.zone3 = DIAGRAL_STATE_DISARMED;
        if (statusFrame.data[6] & DIAGRAL_DATA_ZONE4)
          sDeviceState.zone4 = DIAGRAL_STATE_DISARMED;
      }
      if (statusFrame.data[4] == DIAGRAL_DATA_POWER_LOST)
      {
        sDeviceState.power = DiagralPowerSupply::POWER_NO_MAINS;
      }
      else if (statusFrame.data[4] == 0x00)
      {
        sDeviceState.power = DiagralPowerSupply::POWER_MAINS;
      }
      sDeviceState.lastStateTimestamp = esp_timer_get_time();
      break;
    case SUBCMD_GEN_ALERT_NOTIFICATION:
      if (statusFrame.data_length < 11)
      {
        DIAG_LOGE("UpdateDeviceState: invalid frame length for J=0x{:02X}!", statusFrame.data[1]);
        return;
      }
      sDeviceState.lastAlert.type = (DiagralAlertType)statusFrame.data[3];
      sDeviceState.lastAlert.commandNumber = statusFrame.data[7];
      time(&sDeviceState.lastAlert.timestamp);
      break;
    case SUBCMD_GEN_DETECTION_NOTIFICATION:
      if (statusFrame.data_length < 11)
      {
        DIAG_LOGE("UpdateDeviceState: invalid frame length for J=0x{:02X}!", statusFrame.data[1]);
        return;
      }
      sDeviceState.lastDetection.eventType = (DiagralDetectionEventType)statusFrame.data[3];
      sDeviceState.lastDetection.sensorType = (DiagralSensorType)statusFrame.data[8];
      sDeviceState.lastDetection.sensorNumber = statusFrame.data[9];
      // update zones state whatever detection type
      if (statusFrame.data[4] & DIAGRAL_DATA_ZONE1)
        sDeviceState.zone1 = DIAGRAL_STATE_TRIGGERED;
      if (statusFrame.data[4] & DIAGRAL_DATA_ZONE2)
        sDeviceState.zone2 = DIAGRAL_STATE_TRIGGERED;
      if (statusFrame.data[4] & DIAGRAL_DATA_ZONE3)
        sDeviceState.zone3 = DIAGRAL_STATE_TRIGGERED;
      if (statusFrame.data[4] & DIAGRAL_DATA_ZONE4)
        sDeviceState.zone4 = DIAGRAL_STATE_TRIGGERED;
      time(&sDeviceState.lastDetection.timestamp);
      sDeviceState.lastStateTimestamp = esp_timer_get_time();
      break;
    case SUBCMD_GEN_TAMPERING_NOTIFICATION:
      if (statusFrame.data_length < 10)
      {
        DIAG_LOGE("UpdateDeviceState: invalid frame length for J=0x{:02X}!", statusFrame.data[1]);
        return;
      }
      sDeviceState.lastTamper.isActive = statusFrame.data[3] != 0x00;
      sDeviceState.lastTamper.sensorNumber = statusFrame.data[7];
      time(&sDeviceState.lastTamper.timestamp);
      break;
    default:
      DIAG_LOGE("UpdateDeviceState: unexpected frame J=0x{:02X}!", statusFrame.data[1]);
      break;
    }
    if (!xQueueSendToBack(sDeviceStateQueue, &sDeviceState, 0))
    {
      DIAG_LOGE("UpdateDeviceState: can't add device state to queue!");
    }
  }

} // namespace Diagral
