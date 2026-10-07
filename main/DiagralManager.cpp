#include "DiagralManager.hpp"
#include "HardwareConfig.hpp"
#include "NetworkHelpers.hpp"
#include "MqttConfig.hpp"
#include "SyslogConfig.hpp"
#include "DiagralConfig.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"

static const char *TAG = "diagralMan";

constexpr TickType_t ROLLBACK_TIMEOUT_MS = 60000;           // 60 seconds
constexpr TickType_t MQTT_DISCONNECTED_TIMEOUT_MS = 300000; // 5 minutes
constexpr TickType_t CONNECTIVITY_WATCHDOG_CHECK_MS = 5000; // 5 seconds

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#include <format>

#define DIAG_LOGE(a, ...)                                                              \
    do                                                                                 \
    {                                                                                  \
        loggerCallback(ESP_LOG_ERROR, TAG, std::format(a __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)
#define DIAG_LOGI(a, ...)                                                             \
    do                                                                                \
    {                                                                                 \
        loggerCallback(ESP_LOG_INFO, TAG, std::format(a __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)

using namespace Helpers;
using namespace Config;

namespace Diagral
{
    static DiagralManager *sDiagralManager; // Pointer to DiagralManager instance
    static MqttHelpers *sMqttHelper;        // Pointer to MQTT instance to manage MQTT communication layer
    static SyslogHelpers *sSyslogHelper;    // Pointer to MQTT instance to manage logs to send to syslog server

    /// @brief Callback to manage logs to send to serial console and syslog server
    /// @param log_level log level
    /// @param tag log tag
    /// @param log log content
    static void loggerCallback(esp_log_level_t log_level, const char *tag, std::string log)
    {
        switch (log_level)
        {
        case ESP_LOG_ERROR:
            ESP_LOGE(tag, "%s", log.c_str());
            break;
        case ESP_LOG_INFO:
            ESP_LOGI(tag, "%s", log.c_str());
            break;
        default:
            break;
        }
        // send over syslog
        if (sSyslogHelper != nullptr)
            sSyslogHelper->Send(log_level, tag, log);
    }

    static void deviceStateCallback(const Diagral::DiagralDeviceState &state)
    {
        DIAG_LOGI("Callback received device status");
        if (sDiagralManager == nullptr)
            return;
        if (sDiagralManager->mDiagralDeviceState.lastStateTimestamp != state.lastStateTimestamp)
        {
            DIAG_LOGI("State updated: Mode={}, {}Zone1={}, Zone2={}, Zone3={}, Zone4={}",
                      DiagralModeToString(state.mode),
                      state.error ? "Error detected! ," : "",
                      DiagralStateToString(state.zone1),
                      DiagralStateToString(state.zone2),
                      DiagralStateToString(state.zone3),
                      DiagralStateToString(state.zone4));
        }
        if (sDiagralManager->mDiagralDeviceState.lastAlert.timestamp != state.lastAlert.timestamp)
        {
            DIAG_LOGI("Alert updated: Type={}, Command={}", DiagralAlertTypeToString(state.lastAlert.type), state.lastAlert.commandNumber);
        }
        if (sDiagralManager->mDiagralDeviceState.lastDetection.timestamp != state.lastDetection.timestamp)
        {
            DIAG_LOGI("Detection updated: Event={}, Sensor type={}, num={}",
                      DiagralDetectionEventTypeToString(state.lastDetection.eventType),
                      DiagralSensorTypeToString(state.lastDetection.sensorType),
                      state.lastDetection.sensorNumber);
        }
        if (sDiagralManager->mDiagralDeviceState.lastError.timestamp != state.lastError.timestamp)
        {
            DIAG_LOGI("Error updated: ErrorType={}, HwType={}, num={}",
                      DiagralErrorTypeToString(state.lastError.errorType),
                      DiagralErrorHardwareTypeToString(state.lastError.hardwareType),
                      state.lastError.hardwareNumber);
        }
        if (sDiagralManager->mDiagralDeviceState.battery != state.battery)
        {
            DIAG_LOGI("Battery updated: {}%", state.battery);
        }
        memcpy((void *)&sDiagralManager->mDiagralDeviceState, (void *)&state, sizeof(state));
        // send updated state to MQTT
        if (sMqttHelper != nullptr)
        {
            sMqttHelper->UpdateAndSendDeviceState(state);
        }
    }

    static void networkConnectedCallback()
    {
        // Initialize syslog object
        sDiagralManager->InitializeSyslog();
        // Initialize MQTT object
        sDiagralManager->InitializeMqtt();
        // Start everything
        if (sMqttHelper != nullptr)
        {
            sMqttHelper->StartMqttClient(sDiagralManager, loggerCallback);
        }
    }

    /// @brief Task that checks network and MQTT connection success
    /// @details In case of failure at boot during more than 60s and OTA validation is pending, it will rollback and reboot.
    /// In case of failure during more than 5 minutes, it will send a notification to Diagral alarm system and try to reboot.
    /// @param arg not used
    static void connectivity_watchdog_task(void *arg)
    {
        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
        {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY && esp_ota_check_rollback_is_possible())
            {
                DIAG_LOGI("OTA validation is pending, wait for 60s!");
                vTaskDelay(pdMS_TO_TICKS(ROLLBACK_TIMEOUT_MS));
                if (sDiagralManager->GetTimeSinceMqttDisconnection() < ROLLBACK_TIMEOUT_MS / 1000)
                {
                    DIAG_LOGI("Rollback cancelled!");
                    esp_ota_mark_app_valid_cancel_rollback();
                }
                else
                {
                    DIAG_LOGE("Rollback and reboot!");
                    esp_ota_mark_app_invalid_rollback();
                    sDiagralManager->Reboot();
                }
            }
            else
            {
                DIAG_LOGI("No need to validate OTA!");
            }
        }
        else
        {
            DIAG_LOGE("Unable to get OTA state!");
        }
        if (MqttConfig::isEnabled())
        {
            while (true)
            {
                vTaskDelay(pdMS_TO_TICKS(CONNECTIVITY_WATCHDOG_CHECK_MS));
                if (sDiagralManager->GetTimeSinceMqttDisconnection() > MQTT_DISCONNECTED_TIMEOUT_MS / 1000)
                {
                    DIAG_LOGI("MQTT disconnected since more than 5 minutes -> reboot!");
                    sDiagralManager->mDiagralController->NotifyConnectivity(true);
                    sDiagralManager->Reboot();
                }
            }
        }
        vTaskDelete(NULL);
    }

    DiagralManager::DiagralManager()
    {
        sDiagralManager = this;
        mLastMqttDisconnectionTimestamp = esp_timer_get_time();
        // start connectivity watchdog task
        xTaskCreate(connectivity_watchdog_task, "check_ota_rollback_task", 4096, NULL, tskIDLE_PRIORITY, NULL);
        // Initialize Diagral object
        InitializeDiagral();
        // Initialize network: Ethernet/Wifi + DHCP/Static IP + SNTP
        NetworkHelpers::InitNetwork(networkConnectedCallback);
    }
    void DiagralManager::Reboot()
    {
        esp_restart();
    }
    void DiagralManager::Upgrade(std::string url)
    {
        DIAG_LOGI("Starting OTA");
        esp_http_client_config_t config = {};
        config.url = url.c_str();
        config.cert_pem = (char *)server_cert_pem_start;
        config.keep_alive_enable = true;
#if CONFIG_MBEDTLS_DYNAMIC_BUFFER
        config.tls_dyn_buf_strategy = HTTP_TLS_DYN_BUF_RX_STATIC;
#endif
        config.skip_cert_common_name_check = false;

        esp_https_ota_config_t ota_config = {};
        ota_config.http_config = &config;
        ota_config.ota_resumption = false;

        DIAG_LOGI("Attempting to download update from {}", url);
        esp_err_t ret = esp_https_ota(&ota_config);
        if (ret == ESP_OK)
        {
            DIAG_LOGI("OTA Succeed, Rebooting...");
            Reboot();
        }
        else
        {
            DIAG_LOGE("Firmware upgrade failed");
        }
    }
    void DiagralManager::NotifyMQTTConnectionState(bool connected)
    {
        if (connected)
        {
            mLastMqttDisconnectionTimestamp = 0;
        }
        else if (mLastMqttDisconnectionTimestamp == 0)
        {
            mLastMqttDisconnectionTimestamp = esp_timer_get_time();
        }
    }
    uint32_t DiagralManager::GetTimeSinceMqttDisconnection()
    {
        return mLastMqttDisconnectionTimestamp == 0 ? 0 : (esp_timer_get_time() - mLastMqttDisconnectionTimestamp) / 1000000;
    }
    void DiagralManager::InitializeDiagral()
    {
        // Initialize Diagral controller
        mDiagralController = new Diagral::DiagralController(loggerCallback, deviceStateCallback);
        if (mDiagralController != nullptr)
        {
            mDiagralController->SetVerbose(DiagralConfig::isLoggingEnabled());
            mDiagralController->Init(CONFIG_DIAGRAL_UART_PORT_NUM, CONFIG_DIAGRAL_UART_TXD, CONFIG_DIAGRAL_UART_RXD, CONFIG_DIAGRAL_TX_SIGNAL,
                                     DiagralConfig::isPassiveModeEnabled());
#ifdef CONFIG_DIAGRAL_BATTERY_ENABLED
            BatteryMonitorConfig config = {
                .adc_unit = (adc_unit_t)(CONFIG_DIAGRAL_BATTERY_ADC_UNIT - 1),
                .adc_channel = (adc_channel_t)CONFIG_DIAGRAL_BATTERY_ADC_CHANNEL,
                .min_voltage_mv = CONFIG_DIAGRAL_BATTERY_VOLTAGE_MIN,
                .max_voltage_mv = CONFIG_DIAGRAL_BATTERY_VOLTAGE_MAX};
            mDiagralController->MonitorBattery(config);
#endif
        }
    }
    void DiagralManager::InitializeMqtt()
    {
        if (sMqttHelper != nullptr)
            return;
        if (MqttConfig::isEnabled())
        {
            // Create MQTT helper
            sMqttHelper = new MqttHelpers();
        }
        else
        {
            sMqttHelper = nullptr;
        }
    }
    void DiagralManager::InitializeSyslog()
    {
        if (sSyslogHelper != nullptr)
            return;
        if (SyslogConfig::isEnabled())
        {
            // Create Syslog helper
            sSyslogHelper = new SyslogHelpers();
        }
        else
        {
            sSyslogHelper = nullptr;
        }
    }
}
