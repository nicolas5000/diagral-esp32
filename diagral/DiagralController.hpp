#pragma once

#include <string>
#include <mutex>

#include "esp_log_level.h"
#include "esp_err.h"

#include "hal/adc_types.h"

#include "DiagralConstants.hpp"
#include "DiagralCommands.hpp"
#include "DiagralDeviceState.hpp"

namespace Diagral
{

  typedef void (*LoggerCallback)(esp_log_level_t log_level, const char *tag, std::string log); // Callback to receive logs from the IO controller (if verbose)
  typedef void (*UpdatedDeviceCallback)(const DiagralDeviceState &state);                      // Callback to receive status update

  struct BatteryMonitorConfig
  {
    adc_unit_t adc_unit;       // ADC unit to use
    adc_channel_t adc_channel; // ADC channel to use
    int min_voltage_mv;        // min voltage (in mV), corresponding to 0%
    int max_voltage_mv;        // max voltage (in mV), corresponding to 0%
  };

  /**
   * @brief Diagral UART Controller
   *
   * This class provides a high-level interface for controlling Diagral system from UART.
   * It handles UART communication and protocol details.
   */
  class DiagralController
  {
  public:
    /// @brief Construct a new DiagralController object
    /// @param logger Logging function
    /// @param deviceStateCallback Device state processing callback
    DiagralController(LoggerCallback logger, UpdatedDeviceCallback deviceStateCallback);

    /// @brief Initialize the controller
    /// @param port UART port to use for communication with Diagral system
    /// @param tx GPIO pin to use as UART TX for communication with Diagral system
    /// @param rx GPIO pin to use as UART RX for communication with Diagral system
    /// @param signal GPIO pin to use as TX signal for communication with Diagral system
    /// @param isPassive true to not process received frames internally (listening mode)
    /// @return true on success, false on error
    bool Init(int port, int tx, int rx, int signal, bool isPassive = false);

    /// @brief Launch thread to monitor battery state
    /// @attention Should be called only once!
    /// @param config battery configuration (ADC, min and max voltage)
    void MonitorBattery(BatteryMonitorConfig config);

    /// @brief Enable/disable verbose logging
    /// @param enable true to enable, false to disable
    void SetVerbose(bool enable) { mVerbose = enable; }

    /// @brief Get verbose logging
    /// @return true if verbose logging enabled
    bool isVerbose() { return mVerbose; }

    /// @brief Get active/passive mode. Was set by calling Begin().
    /// @return true if active, false if passive (no radio transmission, only listening)
    bool isPassive() { return mPassiveMode; }

    /// @brief Process received frames
    /// @attention Do not use directly, it is used by internal thread.
    void ProcessReceivedFrameTask();

    /// @brief Update state of the device when necessary or after periodic time
    /// @attention Do not use directly, it is used by internal thread.
    void UpdateDeviceStateTask();

    /// @brief Will force status update of the device
    /// @return true on success, false on error
    bool ForceDeviceStateUpdate();

    /// @brief Send a DiagralFrame and wait for a response.
    /// @param rawFrame string representation of the DiagralFrame, from Identifier byte to last byte of data (without Length, counter and CRC)
    /// @return true if no error
    bool SendRaw(const std::string &rawFrame);

    /// @brief Check PIN code validity in Diagral system
    /// @param pinCode PIN code to check
    /// @return true if PIN code is valid, false otherwise
    bool CheckPinCode(std::string pinCode);

    /// @brief Arm defined zones (1, 2, 3, 4)
    /// @param zones DIAGRAL_DATA_ZONES_ALL or a combination of DIAGRAL_DATA_ZONE1 to DIAGRAL_DATA_ZONE4 (eg for zones 1 + 2, send DIAGRAL_DATA_ZONE1|DIAGRAL_DATA_ZONE2)
    /// @return true if no error
    bool ArmAway(uint8_t zones);

    /// @brief Arm "Home" zones as defined in Diagral system
    /// @return true if no error
    bool ArmHome();

    /// @brief Disarm defined zones (1, 2, 3, 4)
    /// @param zones DIAGRAL_DATA_ZONES_ALL or a combination of DIAGRAL_DATA_ZONE1 to DIAGRAL_DATA_ZONE4 (eg for zones 1 + 2, send DIAGRAL_DATA_ZONE1|DIAGRAL_DATA_ZONE2)
    /// @return true if no error
    bool Disarm(uint8_t zones);

  protected:
    int mTxSignalPin;      // GPIO connected to TX signal pin
    bool mInitialized;     // true if Init is done
    bool mVerbose;         // true if verbose mode (logs are sent to registered callback)
    bool mPassiveMode;     // true if passive mode (will not send frames to UART, only listening)
    std::mutex mUartMutex; // Mutex to protect access to UART

    /// @brief Configure TX Signal pin to enable or disable Tx mode
    /// @param txMode true before emitting (set pin to output high state) or false after emitting (input pulldown)
    /// @return ESP_OK if no error
    esp_err_t ConfigureTxSignalPin(bool txMode);

    /// @brief Compute CRC for given frame
    /// @param buffer Buffer containing frame from Length byte to last byte before CRC
    /// @param length Frame length / Buffer length
    /// @return CRC byte
    uint8_t ComputeCRC(uint8_t buffer[], uint8_t length);

    /// @brief Transmit a frame and wait for ACK
    /// @warning You must take sMutex before calling! Call ManageTxFlagGPIO before and after if necessary!
    /// @param frame Frame to transmit
    /// @return true on success, false on error
    bool TransmitFrame(const DiagralFrame &frame);

    /// @brief Receive a frame and send an ACK
    /// @warning You must take sMutex before calling!
    /// @param frame Received frame
    /// @return true if success (frame available), false otherwise.
    bool ReceiveFrame(DiagralFrame &frame);

    /// @brief Transmit a frame, wait for ACK, then receive a response frame and send an ACK
    /// @param txFrame Frame to transmit
    /// @param rxFrame Received frame
    /// @return true if success (frame available), false otherwise.
    bool SendAndReceive(const DiagralFrame &txFrame, DiagralFrame &rxFrame);

    /// @brief Get or Set current state
    /// @param command Command to execute: DIAGRAL_DATA_STATE_SET_DISARM_ALL, DIAGRAL_DATA_STATE_SET_ARM_ALL,
    /// DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL, DIAGRAL_DATA_STATE_SET_ARM_HOME or DIAGRAL_DATA_STATE_GET
    /// @param disarmZones Zones to disarm (in case of DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL command)
    /// @param armZones Zones to arm (in case of DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL command)
    /// @return true on success
    bool SetGetState(uint8_t command, uint8_t disarmZones = 0, uint8_t armZones = 0);

    /// @brief Update internal device state from received frame and send to callback
    /// @param statusFrame Received state frame
    void UpdateDeviceState(const DiagralFrame &statusFrame);
  };

} // namespace Diagral
