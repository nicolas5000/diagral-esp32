#pragma once

#include <string>

#include "esp_err.h"

namespace Config
{
    class MqttConfig
    {
    public:
        /// @brief Delete all MQTT configuration in storage
        static void DeleteMqttConfig();

        /// @brief Get MQTT activation status from storage
        /// @return true if MQTT enabled
        static bool isEnabled();

        /// @brief Set MQTT activation to configuration storage
        /// @param mqttEnabled true to enable MQTT client
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t Activate(bool mqttEnabled);

        /// @brief Get MQTT broker address from configuration storage
        /// @return broker address
        static const std::string GetBrokerAddress();

        /// @brief Set MQTT broker address to configuration storage
        /// @param address broker address
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetBrokerAddress(const std::string &address);

        /// @brief Get MQTT broker port from configuration storage
        /// @return broker port
        static uint16_t GetBrokerPort();

        /// @brief Set MQTT broker port to configuration storage
        /// @param port broker port
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetBrokerPort(const uint16_t port);

        /// @brief Get MQTT client username from configuration storage
        /// @return client username
        static const std::string GetClientUsername();

        /// @brief Set MQTT client username to configuration storage
        /// @param username client username
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetClientUsername(const std::string &username);

        /// @brief Get MQTT client password from configuration storage
        /// @return client password
        static const std::string GetClientPassword();

        /// @brief Set MQTT client password to configuration storage
        /// @param password client password
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetClientPassword(const std::string &password);

        /// @brief Get TLS activation status from storage
        /// @return true if TLS enabled
        static bool isTLSEnabled();

        /// @brief Set TLS activation to configuration storage
        /// @param tlsEnabled true to enable TLS
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t EnableTLS(bool tlsEnabled);

        /// @brief Get MQTT broker's TLS certificate from configuration storage
        /// @return  broker's TLS certificate
        static const std::string GetBrokerCertificate();

        /// @brief Set MQTT broker's TLS certificate to configuration storage
        /// @param cert broker's TLS certificate
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetBrokerCertificate(const std::string &cert);

        /// @brief Get ID/Name to use in discovery message from configuration storage
        /// @return ID/Name to use
        static const std::string GetDiscoveryIdName();

        /// @brief Set ID/Name to use in discovery message to configuration storage
        /// @param id ID/Name
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetDiscoveryIdName(const std::string &id);

        /// @brief Get MQTT topic prefix from configuration storage
        /// @return topic prefix
        static const std::string GetTopicPrefix();

        /// @brief Set MQTT topic prefix to configuration storage
        /// @param prefix topic prefix
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetTopicPrefix(const std::string &prefix);

        /// @brief Get MQTT discovery prefix from configuration storage
        /// @return discovery prefix
        static const std::string GetDiscoveryPrefix();

        /// @brief Set MQTT discovery prefix to configuration storage
        /// @param prefix discovery prefix
        /// @return ESP_OK if configuration put to storage without error
        static esp_err_t SetDiscoveryPrefix(const std::string &prefix);
    };

}