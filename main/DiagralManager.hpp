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

        /// @brief Launch OTA firmware upgrade using provided URL to get firmware
        /// @param url URL to the upgrade file
        void Upgrade(std::string url);

        /// @brief Initialize MQTT object members (sMqttHelper)
        void InitializeMqtt();

        /// @brief Initialize Syslog object members (sSyslogHelper)
        void InitializeSyslog();

        /// @brief Called by MqttHelpers when connected to or disconnected from MQTT server
        /// @param connected true if MQTT is connected, false if MQTT is disconnected
        void NotifyMQTTConnectionState(bool connected);

        /// @brief Retrieve current configuration about passive / active mode
        /// @return true if currently in passive mode
        bool isDiagralPassive() { return mDiagralPassive; }

        /// @brief Retrieve time elapsed since MQTT is disconnected
        /// @return time in seconds
        uint32_t GetTimeSinceMqttDisconnection();

    private:
        bool mDiagralPassive = false; // current configuration, initialized at boot
        uint64_t mLastMqttDisconnectionTimestamp; // contains the timestamp when MQTT was disconnected for the last time (or 0 if currently connected)

        /// @brief Initialize Diagral controller member (mDiagralController)
        void InitializeDiagral();
    };

}