#pragma once

#include "MqttHelpers.hpp"
#include "SyslogHelpers.hpp"
#include "DiagralController.hpp"

namespace Diagral
{
    class DiagralManager
    {
    public:
        DiagralDeviceState mDiagralDeviceState; // Currently managed Diagral Device state
        DiagralController *mDiagralController;  // Pointer to DiagralController object used to manage Diagral system from UART

        /// @brief Constructor for DiagralManager
        DiagralManager();

        /// @brief Ask to reboot ESP32
        void Reboot();

        /// @brief Retrieve current configuration about passive / active mode
        /// @return true if currently in passive mode
        bool isDiagralPassive() { return mDiagralPassive; }

    private:
        bool mDiagralPassive = false; // current configuration, initialized at boot

        /// @brief Initialize Diagral controller member (mDiagralController)
        void InitializeDiagral();

        /// @brief Initialize MQTT object members (sMqttHelper)
        void InitializeMqtt();

        /// @brief Initialize Syslog object members (sSyslogHelper)
        void InitializeSyslog();
    };

}