#include "DiagralConfig.hpp"
#include "NvsHelpers.hpp"

#include "sdkconfig.h"

static const std::string DIAGRAL_CONFIG_NAMESPACE = "diagral";              // namespace to group IO HomeControl configuration in NVS
static const std::string DIAGRAL_CONFIG_LOGGING_ENABLE = "log_enabled";     // Key to store logging status (uint8_t), 0 if logging disabled
static const std::string DIAGRAL_CONFIG_PASSIVE_ENABLE = "passive_enabled"; // Key to store passive mode status (uint8_t), 0 if passive mode disabled
static const std::string DIAGRAL_CONFIG_PIN_CODE = "pin_code";              // Key to store PIN code (string representation)

using namespace Helpers;

namespace Config
{
    void DiagralConfig::DeleteDiagralConfig()
    {
        NvsHelpers::DeleteValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_LOGGING_ENABLE);
        NvsHelpers::DeleteValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_PASSIVE_ENABLE);
        NvsHelpers::DeleteValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_PIN_CODE);
    }
    bool DiagralConfig::isLoggingEnabled()
    {
#ifdef CONFIG_DIAGRAL_UART_LOGGING_ENABLED
        uint8_t is_enabled = true;
#else
        uint8_t is_enabled = false;
#endif
        NvsHelpers::GetValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_LOGGING_ENABLE, is_enabled);
        return is_enabled;
    }
    esp_err_t DiagralConfig::ActivateLogging(bool loggingEnabled)
    {
        uint8_t is_enabled = loggingEnabled ? 0x01 : 0x00;
        return NvsHelpers::SetValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_LOGGING_ENABLE, is_enabled);
    }
    bool DiagralConfig::isPassiveModeEnabled()
    {
#ifdef CONFIG_DIAGRAL_UART_PASSIVE_MODE
        uint8_t is_enabled = true;
#else
        uint8_t is_enabled = false;
#endif
        NvsHelpers::GetValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_PASSIVE_ENABLE, is_enabled);
        return is_enabled;
    }
    esp_err_t DiagralConfig::ActivatePassiveMode(bool passiveModeEnabled)
    {
        uint8_t is_enabled = passiveModeEnabled ? 0x01 : 0x00;
        return NvsHelpers::SetValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_PASSIVE_ENABLE, is_enabled);
    }
    bool DiagralConfig::isPinCodeCheckEnabled()
    {
#ifdef CONFIG_DIAGRAL_MQTT_COMMAND_CHECK_PIN
        uint8_t is_enabled = true;
#else
        uint8_t is_enabled = false;
#endif
        return NvsHelpers::GetValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_PIN_CODE, is_enabled);
    }
    esp_err_t DiagralConfig::ActivatePinCodeCheck(bool pinCodeCheckEnabled)
    {
        uint8_t is_enabled = pinCodeCheckEnabled ? 0x01 : 0x00;
        return NvsHelpers::SetValue(DIAGRAL_CONFIG_NAMESPACE, DIAGRAL_CONFIG_PIN_CODE, is_enabled);
    }
}