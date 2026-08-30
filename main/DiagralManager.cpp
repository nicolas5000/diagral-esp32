#include "DiagralManager.hpp"
#include "HardwareConfig.hpp"
#include "MqttConfig.hpp"
#include "SyslogConfig.hpp"
#include "DiagralConfig.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "diagralMan";

#include <format>

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
        ESP_LOGI(TAG, "Callback received device status");
        if (sDiagralManager == nullptr)
            return;
        if (sDiagralManager->mDiagralDeviceState.lastStateTimestamp != state.lastStateTimestamp)
        {
            ESP_LOGI(TAG, "State updated: Mode=0x%02X, Zone1=0x%02X, Zone2=0x%02X, Zone3=0x%02X, Zone4=0x%02X", state.mode, state.zone1, state.zone2, state.zone3, state.zone4);
        }
        if (sDiagralManager->mDiagralDeviceState.lastAlert.timestamp != state.lastAlert.timestamp)
        {
            ESP_LOGI(TAG, "Alert updated: Type=0x%02X, Command=%d", state.lastAlert.type, state.lastAlert.commandNumber);
        }
        if (sDiagralManager->mDiagralDeviceState.lastDetection.timestamp != state.lastDetection.timestamp)
        {
            ESP_LOGI(TAG, "Detection updated: Event=0x%02X, Sensor type/number=0x%02X/%d", state.lastDetection.eventType, state.lastDetection.sensorType, state.lastDetection.sensorNumber);
        }
        if (sDiagralManager->mDiagralDeviceState.lastTamper.timestamp != state.lastTamper.timestamp)
        {
            ESP_LOGI(TAG, "Tamper updated: Sensor=%d, isActive=%s", state.lastTamper.sensorNumber, state.lastTamper.isActive ? "Yes" : "No");
        }
        if (sDiagralManager->mDiagralDeviceState.mode != state.mode)
        {
            ESP_LOGI(TAG, "Mode updated: %s", DiagralModeToString(state.mode).c_str());
        }
        if (sDiagralManager->mDiagralDeviceState.power != state.power)
        {
            ESP_LOGI(TAG, "Power updated: %s", DiagralPowerSupplyToString(state.power).c_str());
        }
        if (sDiagralManager->mDiagralDeviceState.battery != state.battery)
        {
            ESP_LOGI(TAG, "Battery updated: %d%", state.battery);
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
            sMqttHelper->StartMqttClient();
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
            sMqttHelper = new MqttHelpers(this);
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
