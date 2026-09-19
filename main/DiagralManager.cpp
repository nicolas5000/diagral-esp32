#include "DiagralManager.hpp"
#include "HardwareConfig.hpp"
#include "MqttConfig.hpp"
#include "SyslogConfig.hpp"
#include "DiagralConfig.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "diagralMan";

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

    DiagralManager::DiagralManager()
    {
        sDiagralManager = this;
        // Initialize Diagral object
        InitializeDiagral();
        // Initialize MQTT object
        InitializeMqtt();
        // Initialize syslog object
        InitializeSyslog();
        // Start everything
        if (sMqttHelper != nullptr)
        {
            sMqttHelper->StartMqttClient(this, loggerCallback);
        }
    }
    void DiagralManager::Reboot()
    {
        esp_restart();
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
