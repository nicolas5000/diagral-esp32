#pragma once

#include <string>

#include "esp_err.h"
#include "esp_timer.h"

#include "DiagralManager.hpp"
#include "mqtt_client.h"

// forward declaration
namespace Diagral
{
    class DiagralManager;
    struct DiagralDeviceState;
    struct DiagralAlert;
    struct DiagralDetection;
    struct DiagralDetectionEvent;
    struct DiagralTamper;
}

namespace Helpers
{
    class MqttHelpers
    {
    public:
        /// @brief Construct a new MqttHelpers object
        /// @param manager Pointer to IoRtsManager object
        MqttHelpers(Diagral::DiagralManager *manager);
        /// @brief Start MQTT client
        /// @return ESP_OK if no error, ESP_ERR_NOT_ALLOWED if MQTT is not enabled in configuration or already started, ...
        esp_err_t StartMqttClient();

        /// @brief Send discovery messages compatible with Home Assistant
        /// Sends a controller device discovery and a separate discovery for each IO device (linked via via_device)
        void SendDiscovery();

        /// @brief Update and send MQTT device state messages over MQTT
        /// @param state up-to-date device state
        void UpdateAndSendDeviceState(const Diagral::DiagralDeviceState &state);

        /// @brief Send MQTT device state messages (info, zones, last events) over MQTT
        /// @param info if true, send info message
        /// @param panel if true, send control_panel message
        /// @param detection if true, send last_detection message
        /// @param alert if true, send last_alert message
        /// @param tamper if true, send last_tamper message
        void SendDeviceState(bool info, bool panel, bool detection, bool alert, bool tamper);

        const std::string &GetTopicPrefix() { return mTopicPrefix; }

        /// @brief Returns pointer to DiagralManager instance
        /// @return pointer to DiagralManager instance
        Diagral::DiagralManager *GetDiagralManager() { return mDiagralManager; }

        /// @brief Returns Diagral controller passive mode
        /// @return true if Diagral controller is in passive mode
        bool isDiagralPassive() { return mIsDiagralPassive; }

        /// @brief Returns Diagral PIN check status
        /// @return true if Diagral PIN check is enabled
        bool isPinCheckEnabled() { return mIsPinCheckEnabled; }

        /// @brief Called when network IP is obtained — triggers MQTT reconnect
        void OnNetworkConnected();

        /// @brief Called when network link drops — cancels any pending MQTT reconnect timer
        void OnNetworkDisconnected();

        /// @brief Called on MQTT_EVENT_DISCONNECTED — schedules reconnect if network is up
        void OnMqttDisconnected();

    private:
        Diagral::DiagralManager *mDiagralManager;   // Pointer to DiagralManager object
        bool mStarted;                              // true if client is started
        bool mIsDiagralPassive;                     // true if Diagral is in passive mode
        bool mIsPinCheckEnabled;                    // true if PIN check is enabled
        std::string mTopicPrefix;                   // Topic prefix, initialized from configuration storage at boot (avoid to read it from storage everytime!)
        std::string mDiscoveryPrefix;               // Discovery prefix, initialized from configuration storage at boot (avoid to read it from storage everytime!)
        esp_mqtt_client_handle_t mMqttClientHandle; // Handle on MQTT client
        esp_timer_handle_t mReconnectTimer;         // One-shot timer for broker-drop reconnect
    };
}