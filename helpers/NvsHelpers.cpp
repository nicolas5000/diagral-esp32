#include "NvsHelpers.hpp"

#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"

#include "esp_log.h"

static std::unique_ptr<nvs::NVSHandle> sNvsHandle = nullptr;

static const char *TAG = "helpers";

namespace Helpers
{
    esp_err_t NvsHelpers::InitNvs()
    {
        // Initialize NVS
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
        return err;
    }
    
    template <typename T>
    esp_err_t NvsHelpers::GetValue(const std::string &name_space, const std::string &key, T &value)
    {
        esp_err_t err = ESP_OK;
        if (sNvsHandle == nullptr)
            sNvsHandle = nvs::open_nvs_handle(name_space.c_str(), NVS_READWRITE, &err);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            // Read
            err = sNvsHandle->get_item(key.c_str(), value);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            {
                ESP_LOGE(TAG, "Error (%s) reading value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
        }
        return err;
    }

    // Explicit instanciation for authorized types
    template esp_err_t NvsHelpers::GetValue<uint8_t>(const std::string &name_space, const std::string &key, uint8_t &value);
    template esp_err_t NvsHelpers::GetValue<int8_t>(const std::string &name_space, const std::string &key, int8_t &value);
    template esp_err_t NvsHelpers::GetValue<uint16_t>(const std::string &name_space, const std::string &key, uint16_t &value);
    template esp_err_t NvsHelpers::GetValue<int16_t>(const std::string &name_space, const std::string &key, int16_t &value);
    template esp_err_t NvsHelpers::GetValue<uint32_t>(const std::string &name_space, const std::string &key, uint32_t &value);
    template esp_err_t NvsHelpers::GetValue<int32_t>(const std::string &name_space, const std::string &key, int32_t &value);
    template esp_err_t NvsHelpers::GetValue<uint64_t>(const std::string &name_space, const std::string &key, uint64_t &value);
    template esp_err_t NvsHelpers::GetValue<int64_t>(const std::string &name_space, const std::string &key, int64_t &value);

    template <typename T>
    esp_err_t NvsHelpers::SetValue(const std::string &name_space, const std::string &key, const T &value)
    {
        esp_err_t err = ESP_OK;
        if (sNvsHandle == nullptr)
            sNvsHandle = nvs::open_nvs_handle(name_space.c_str(), NVS_READWRITE, &err);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            // Write
            err = sNvsHandle->set_item(key.c_str(), value);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error (%s) writing value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
            // Commit
            err = sNvsHandle->commit();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error (%s) committing value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
        }
        return err;
    }

    // Explicit instanciation for authorized types
    template esp_err_t NvsHelpers::SetValue<uint8_t>(const std::string &name_space, const std::string &key, const uint8_t &value);
    template esp_err_t NvsHelpers::SetValue<int8_t>(const std::string &name_space, const std::string &key, const int8_t &value);
    template esp_err_t NvsHelpers::SetValue<uint16_t>(const std::string &name_space, const std::string &key, const uint16_t &value);
    template esp_err_t NvsHelpers::SetValue<int16_t>(const std::string &name_space, const std::string &key, const int16_t &value);
    template esp_err_t NvsHelpers::SetValue<uint32_t>(const std::string &name_space, const std::string &key, const uint32_t &value);
    template esp_err_t NvsHelpers::SetValue<int32_t>(const std::string &name_space, const std::string &key, const int32_t &value);
    template esp_err_t NvsHelpers::SetValue<uint64_t>(const std::string &name_space, const std::string &key, const uint64_t &value);
    template esp_err_t NvsHelpers::SetValue<int64_t>(const std::string &name_space, const std::string &key, const int64_t &value);

    esp_err_t NvsHelpers::DeleteValue(const std::string &name_space, const std::string &key)
    {
        esp_err_t err = ESP_OK;
        nvs_type_t type;
        if (sNvsHandle == nullptr)
            sNvsHandle = nvs::open_nvs_handle(name_space.c_str(), NVS_READWRITE, &err);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else if (sNvsHandle->find_key(key.c_str(), type) != ESP_OK) return ESP_OK; // Key doesn't exist
        else
        {
            // Delete
            err = sNvsHandle->erase_item(key.c_str());
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error (%s) writing value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
            // Commit
            err = sNvsHandle->commit();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error (%s) committing value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
        }
        return err;
    }

    esp_err_t NvsHelpers::GetString(const std::string &name_space, const std::string &key, std::string &value)
    {
        esp_err_t err = ESP_OK;
        if (sNvsHandle == nullptr)
            sNvsHandle = nvs::open_nvs_handle(name_space.c_str(), NVS_READWRITE, &err);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            // Read
            size_t paramSize;
            err = sNvsHandle->get_item_size(nvs::ItemType::SZ, key.c_str(), paramSize);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            {
                ESP_LOGE(TAG, "Error (%s) reading value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
            else if (err == ESP_ERR_NVS_NOT_FOUND)
                ; // do nothing
            else
            {
                char *tmp = new char[paramSize];
                err = sNvsHandle->get_string(key.c_str(), tmp, paramSize);
                value.assign(tmp);
                delete[] tmp;
            }
        }
        return err;
    }
    esp_err_t NvsHelpers::SetString(const std::string &name_space, const std::string &key, const std::string &value)
    {
        esp_err_t err = ESP_OK;
        if (sNvsHandle == nullptr)
            sNvsHandle = nvs::open_nvs_handle(name_space.c_str(), NVS_READWRITE, &err);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            // Write
            err = sNvsHandle->set_string(key.c_str(), value.c_str());
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error (%s) writing value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
            // Commit
            err = sNvsHandle->commit();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error (%s) committing value for key %s in NVS namespace %s!\n", esp_err_to_name(err), key.c_str(), name_space.c_str());
            }
        }
        return err;
    }
}