#pragma once

#include <stdint.h>
#include <string>
#include "esp_err.h"

namespace Helpers
{
    class NvsHelpers
    {
    public:

        /// @brief Initialize default NVS partition, must be called before any read/write operation
        /// @return ESP_OK if no error
        static esp_err_t InitNvs();

        /// @brief Template to get a value of type T from default NVS partition
        /// @tparam T can be uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t
        /// @param name_space the namespace containing the key/value pair to retrieve
        /// @param key the key associated to the value to retrieve
        /// @param value will be set to the value retrieved from NVS partition. Not modified if value doesn't exist in NVS (so can be initialized to default value)
        /// @return ESP_OK if no error (value retrieved from NVS), ESP_ERR_NVS_NOT_FOUND if the key is not (yet) in NVS partition, or any other error
        template<typename T>
        static esp_err_t GetValue(const std::string &name_space, const std::string &key, T &value);

        /// @brief Template to set a value of type T to default NVS partition
        /// @tparam T can be uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t
        /// @param name_space the namespace containing the key/value pair to store
        /// @param key the key associated to the value to store
        /// @param value value to store associated to key, grouped in namespace
        /// @return ESP_OK if no error (value stored to NVS), or any other error
        template<typename T>
        static esp_err_t SetValue(const std::string &name_space, const std::string &key, const T &value);

        /// @brief Delete a value of any type (including string) in default NVS partition
        /// @param name_space the namespace containing the key/value pair to delete
        /// @param key the key associated to the value to delete
        /// @return ESP_OK if no error (value deleted in NVS), or any other error
        static esp_err_t DeleteValue(const std::string &name_space, const std::string &key);

        /// @brief Get a string value from default NVS partition
        /// @param name_space the namespace containing the key/value pair to retrieve
        /// @param key the key associated to the value to retrieve
        /// @param value will be set to the value retrieved from NVS partition. Not modified if value doesn't exist in NVS (so can be initialized to default value)
        /// @return ESP_OK if no error (value retrieved from NVS), ESP_ERR_NVS_NOT_FOUND if the key is not (yet) in NVS partition, or any other error
        static esp_err_t GetString(const std::string &name_space, const std::string &key, std::string &value);

        /// @brief Template to set a string value to default NVS partition
        /// @param name_space the namespace containing the key/value pair to store
        /// @param key the key associated to the value to store
        /// @param value value to store associated to key, grouped in namespace
        /// @return ESP_OK if no error (value stored to NVS), or any other error
        static esp_err_t SetString(const std::string &name_space, const std::string &key, const std::string &value);
    };

}