#pragma once

#include <string>

#include "esp_err.h"

namespace Config
{
    class DiagralConfig
    {
    public:
        /// @brief Delete all Diagral configuration in storage
        static void DeleteDiagralConfig();

        /// @brief Get logging in Diagral layer status from storage
        /// @return true if logging is enabled
        static bool isLoggingEnabled();

        /// @brief Set logging in Diagral layer to configuration storage
        /// @param loggingEnabled true to enable logging in Diagral layer
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t ActivateLogging(bool loggingEnabled);

        /// @brief Get passive mode Diagral layer status from storage
        /// @return true if passive mode is enabled
        static bool isPassiveModeEnabled();

        /// @brief Set passive mode in Diagral layer to configuration storage
        /// @param passiveModeEnabled true to enable passive mode in Diagral layer
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t ActivatePassiveMode(bool passiveModeEnabled);

        /// @brief Get PIN code check status from configuration storage
        /// @return true if PIN code check is enabled
        static bool isPinCodeCheckEnabled();

        /// @brief Set PIN code check to configuration storage
        /// @param ioSystemKey true to enable PIN mode check
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t ActivatePinCodeCheck(bool pinCodeCheckEnabled);
    };

}