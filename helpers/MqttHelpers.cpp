#include "MqttHelpers.hpp"
#include "MqttConfig.hpp"
#include "DiagralConfig.hpp"
#include "NetworkHelpers.hpp"

#include <algorithm>
#include <time.h>

#include <format>

#include "cJSON.h"

#include "esp_app_desc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

#ifdef CONFIG_CONNECTIVITY_CHOICE_WIFI
#include "esp_wifi.h"
#endif
#ifdef CONFIG_CONNECTIVITY_CHOICE_ETH
#include "esp_eth.h"
#include "esp_netif.h"
#endif

using namespace Config;
using namespace Diagral;

static const std::string MQTT_CERTIFICATE_BEGIN = "-----BEGIN CERTIFICATE-----\n";
static const std::string MQTT_CERTIFICATE_END = "\n-----END CERTIFICATE-----";
static const std::string MQTT_CLIENT_COMMAND_TOPIC = "/set";                   // command topic
static const std::string MQTT_CLIENT_STATE_TOPIC = "/state";                   // state topic
static const std::string MQTT_CLIENT_INFO_TOPIC = "/info";                     // info topic
static const std::string MQTT_CLIENT_CONTROL_PANEL_TOPIC = "/control_panel";   // control panel topic
static const std::string MQTT_CLIENT_LAST_DETECTION_TOPIC = "/last_detection"; // last detection topic
static const std::string MQTT_CLIENT_LAST_ALERT_TOPIC = "/last_alert";         // last alert topic
static const std::string MQTT_CLIENT_LAST_TAMPER_TOPIC = "/last_tamper";       // last tamper topic
static const std::string MQTT_CLIENT_DISCOVERY_TOPIC = "/config";              // config topic

static const std::string MQTT_CLIENT_REBOOT_ID = "button_reboot"; // unique id suffix and topic for "reboot" button

static const std::string MQTT_CLIENT_CONFIG_ID = "config";   // id for configuration components
static const std::string MQTT_CLIENT_LOGGING_ID = "Logging"; // unique id and action for "Logging" component
static const std::string MQTT_CLIENT_PASSIVE_ID = "Passive"; // unique id and action for "Passive" component

static const std::string MQTT_CLIENT_CONTROL_PANEL_ID = "control_panel"; // control panel id
static const std::string MQTT_CLIENT_CONTROL_PANEL_ACTION_ID = "action"; // control panel - action id
static const std::string MQTT_CLIENT_CONTROL_PANEL_CODE_ID = "code";     // control panel - code id
static const std::string MQTT_CLIENT_CONTROL_PANEL_ZONES_ID = "zones";   // control panel - zones id

static const std::string MQTT_CLIENT_LAST_DETECTION_ID = "last_detection"; // unique id suffix for "last_detection" component
static const std::string MQTT_CLIENT_LAST_ALERT_ID = "last_alert";         // unique id suffix for "last_alert" component
static const std::string MQTT_CLIENT_LAST_TAMPER_ID = "last_tamper";       // unique id suffix for "last_tamper" component

static const std::string MQTT_CLIENT_BIRTH_WILL_TOPIC = "/status"; // birth and last will topic
static const std::string MQTT_CLIENT_BIRTH_MSG = "online";         // last will message - birth
static const std::string MQTT_CLIENT_WILL_MSG = "offline";         // last will message - death

static const char *TAG = "MQTTHelper";

static Diagral::DiagralDeviceState sDiagralDeviceState; // Current Diagral device state to update MQTT topics

namespace Helpers
{
    static void mqtt_reconnect_timer_cb(void *arg)
    {
        static_cast<MqttHelpers *>(arg)->OnNetworkConnected();
    }

    static void mqtt_network_event_handler(void *handler_args, esp_event_base_t event_base,
                                           int32_t event_id, void *event_data)
    {
        MqttHelpers *mqttHelper = static_cast<MqttHelpers *>(handler_args);
#ifdef CONFIG_CONNECTIVITY_CHOICE_WIFI
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
            mqttHelper->OnNetworkDisconnected();
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
            mqttHelper->OnNetworkConnected();
#endif
#ifdef CONFIG_CONNECTIVITY_CHOICE_ETH
        if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED)
            mqttHelper->OnNetworkDisconnected();
        else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP)
            mqttHelper->OnNetworkConnected();
#endif
    }

    /// @brief Event handler registered to receive MQTT events
    /// @param handler_args user data registered to the event => MqttHelpers object
    /// @param base Event base for the handler(always MQTT Base)
    /// @param event_id The id for the received event.
    /// @param event_data The data for the event, esp_mqtt_event_handle_t
    static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
    {
        ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
        MqttHelpers *mqttHelper = static_cast<MqttHelpers *>(handler_args);
        esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
        esp_mqtt_client_handle_t client = event->client;
        int msg_id;
        switch ((esp_mqtt_event_id_t)event_id)
        {
        case MQTT_EVENT_CONNECTED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // send birth message
            std::string topic = mqttHelper->GetTopicPrefix() + MQTT_CLIENT_BIRTH_WILL_TOPIC;
            const char *data = MQTT_CLIENT_BIRTH_MSG.c_str();
            msg_id = esp_mqtt_client_publish(client, topic.c_str(), data, 0, 0, 1);
            ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
            // Send current config (logging, passive mode)
            topic = mqttHelper->GetTopicPrefix() + "/" + MQTT_CLIENT_CONFIG_ID + MQTT_CLIENT_STATE_TOPIC;
            cJSON *diagralConfig = cJSON_CreateObject();
            if (diagralConfig != NULL)
            {
                bool error = false;
                std::string logging = DiagralConfig::isLoggingEnabled() ? "ON" : "OFF";
                std::string passive = DiagralConfig::isPassiveModeEnabled() ? "ON" : "OFF";
                error = error || (cJSON_AddStringToObject(diagralConfig, MQTT_CLIENT_LOGGING_ID.c_str(), logging.c_str()) == NULL); // IoLogging
                error = error || (cJSON_AddStringToObject(diagralConfig, MQTT_CLIENT_PASSIVE_ID.c_str(), passive.c_str()) == NULL); // IoPassive
                const char *config = cJSON_Print(diagralConfig);
                esp_mqtt_client_publish(client, topic.c_str(), config, 0, 0, 1);
                cJSON_free((void *)config);
                cJSON_Delete(diagralConfig);
            }
            // Send discovery
            mqttHelper->SendDiscovery();
            // Re-publish device info, current state and last events so HA is immediately up-to-date
            mqttHelper->SendDeviceState(true, true, true, true, true);
            // subscribe to all command topics
            topic = mqttHelper->GetTopicPrefix() + "/+" + MQTT_CLIENT_COMMAND_TOPIC;
            msg_id = esp_mqtt_client_subscribe(client, topic.c_str(), 0);
            break;
        }
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqttHelper->OnMqttDisconnected();
            break;

        case MQTT_EVENT_SUBSCRIBED:
            // ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            // ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
        {
            // ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            // ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            // ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
            // Parse command and do the job!
            // Is it a command for us?
            const std::string topic_str(event->topic, event->topic_len);
            if (topic_str.starts_with(mqttHelper->GetTopicPrefix()) && topic_str.ends_with(MQTT_CLIENT_COMMAND_TOPIC))
            {
                size_t id_len = topic_str.length() - mqttHelper->GetTopicPrefix().length() - MQTT_CLIENT_COMMAND_TOPIC.length() - 1;
                if (id_len > 0)
                {
                    std::string entity_id = topic_str.substr(mqttHelper->GetTopicPrefix().length() + 1, id_len);
                    if (entity_id.compare(MQTT_CLIENT_REBOOT_ID) == 0)
                    {
                        ESP_LOGI(TAG, "REBOOT requested from MQTT!");
                        mqttHelper->GetDiagralManager()->Reboot();
                    }
                    else if (entity_id.compare(MQTT_CLIENT_CONFIG_ID) == 0 && topic_str.ends_with(MQTT_CLIENT_COMMAND_TOPIC))
                    {
                        ESP_LOGI(TAG, "CONFIG requested from MQTT!");
                        // Let's parse the JSON
                        char *buf = new char[event->data_len + 1];
                        memcpy(buf, event->data, event->data_len);
                        buf[event->data_len] = '\0';
                        cJSON *root = cJSON_Parse(buf);
                        delete[] buf;
                        if (root == nullptr)
                        {
                            ESP_LOGE(TAG, "Failed to parse JSON from CONFIG requested from MQTT!");
                            break;
                        }
                        // Let's check what we have to do
                        cJSON *loggingItem = cJSON_GetObjectItem(root, MQTT_CLIENT_LOGGING_ID.c_str());
                        if (cJSON_IsString(loggingItem))
                        {
                            std::string value(loggingItem->valuestring);
                            if (value.compare("ON") == 0)
                            {
                                DiagralConfig::ActivateLogging(true);
                            }
                            else if (value.compare("OFF") == 0)
                            {
                                DiagralConfig::ActivateLogging(false);
                            }
                        }
                        cJSON *passiveItem = cJSON_GetObjectItem(root, MQTT_CLIENT_PASSIVE_ID.c_str());
                        if (cJSON_IsString(passiveItem))
                        {
                            std::string value(passiveItem->valuestring);
                            if (value.compare("ON") == 0)
                            {
                                DiagralConfig::ActivatePassiveMode(true);
                            }
                            else if (value.compare("OFF") == 0)
                            {
                                DiagralConfig::ActivatePassiveMode(false);
                            }
                        }
                        // Don't forget to delete JSON object to free memory!
                        cJSON_Delete(root);
                    }
                    else if (entity_id.compare(MQTT_CLIENT_CONTROL_PANEL_ID) == 0 && topic_str.ends_with(MQTT_CLIENT_COMMAND_TOPIC))
                    {
                        if (mqttHelper->isDiagralPassive())
                            break; // don't process panel commands if in passive mode

                        // Let's parse the JSON
                        bool error = false;
                        char *buf = new char[event->data_len + 1];
                        memcpy(buf, event->data, event->data_len);
                        buf[event->data_len] = '\0';
                        cJSON *root = cJSON_Parse(buf);
                        delete[] buf;
                        if (root == nullptr)
                        {
                            ESP_LOGE(TAG, "Failed to parse JSON from CONTROL_PANEL command!");
                            break;
                        }
                        // Let's check parameters
                        std::string action, code;
                        double zones;
                        cJSON *actionItem = cJSON_GetObjectItem(root, MQTT_CLIENT_CONTROL_PANEL_ACTION_ID.c_str());
                        if (cJSON_IsString(actionItem))
                        {
                            action = std::string(actionItem->valuestring);
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Failed to extract action field from CONTROL_PANEL command!");
                            error = true;
                        }
                        cJSON *codeItem = cJSON_GetObjectItem(root, MQTT_CLIENT_CONTROL_PANEL_CODE_ID.c_str());
                        if (cJSON_IsString(codeItem))
                        {
                            code = std::string(codeItem->valuestring);
                        }
                        else if (mqttHelper->isPinCheckEnabled())
                        {
                            ESP_LOGE(TAG, "Failed to extract code field from CONTROL_PANEL command!");
                            error = true;
                        }
                        cJSON *zonesItem = cJSON_GetObjectItem(root, MQTT_CLIENT_CONTROL_PANEL_ZONES_ID.c_str());
                        if (cJSON_IsNumber(zonesItem))
                        {
                            zones = zonesItem->valuedouble;
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Failed to extract zones field from CONTROL_PANEL command!");
                            error = true;
                        }
                        // First check PIN code if enabled
                        if (!error && mqttHelper->isPinCheckEnabled())
                        {
                            if (!mqttHelper->GetDiagralManager()->mDiagralController->CheckPinCode(code))
                            {
                                ESP_LOGE(TAG, "Failed to validate PIN code from CONTROL_PANEL command!");
                                break;
                            }
                        }
                        // Then apply command
                        if (!error)
                        {
                            if (action.compare("ARM_HOME") == 0)
                                mqttHelper->GetDiagralManager()->mDiagralController->ArmHome();
                            else if (action.compare("ARM_AWAY") == 0)
                                mqttHelper->GetDiagralManager()->mDiagralController->ArmAway(zones);
                            else if (action.compare("DISARM") == 0)
                                mqttHelper->GetDiagralManager()->mDiagralController->Disarm(zones);
                        }
                        // Don't forget to delete JSON object to free memory!
                        cJSON_Delete(root);
                    }
                }
            }
            break;
        }
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            }
            else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
            {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            }
            else
            {
                ESP_LOGE(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
        }
    }

    MqttHelpers::MqttHelpers(Diagral::DiagralManager *manager)
        : mDiagralManager(manager), mStarted(false), mMqttClientHandle(nullptr), mReconnectTimer(nullptr)
    {
        mIsDiagralPassive = DiagralConfig::isPassiveModeEnabled();
        mIsPinCheckEnabled = DiagralConfig::isPinCodeCheckEnabled();
        mTopicPrefix = MqttConfig::GetTopicPrefix();
        mDiscoveryPrefix = MqttConfig::GetDiscoveryPrefix();
    }
    esp_err_t MqttHelpers::StartMqttClient()
    {
        if (!MqttConfig::isEnabled() || mStarted)
            return ESP_ERR_NOT_ALLOWED;
        esp_err_t err = ESP_OK;
        // Configure client
        esp_mqtt_client_config_t mqtt_cfg;
        memset(&mqtt_cfg, 0, sizeof(esp_mqtt_client_config_t));
        mqtt_cfg.broker.address.hostname = MqttConfig::GetBrokerAddress().c_str();
        mqtt_cfg.broker.address.port = MqttConfig::GetBrokerPort();
        bool tls_enabled = MqttConfig::isTLSEnabled();
        mqtt_cfg.broker.address.transport = tls_enabled ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP;
        if (tls_enabled)
        {
            mqtt_cfg.broker.verification.certificate = std::string(MQTT_CERTIFICATE_BEGIN + MqttConfig::GetBrokerCertificate() + MQTT_CERTIFICATE_END).c_str();
            mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
        }
        mqtt_cfg.credentials.username = MqttConfig::GetClientUsername().c_str();
        mqtt_cfg.credentials.authentication.password = MqttConfig::GetClientPassword().c_str();
        mqtt_cfg.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
        mqtt_cfg.session.last_will.topic = std::string(MqttConfig::GetTopicPrefix() + MQTT_CLIENT_BIRTH_WILL_TOPIC).c_str();
        mqtt_cfg.session.last_will.msg = MQTT_CLIENT_WILL_MSG.c_str();
        mqtt_cfg.session.last_will.msg_len = MQTT_CLIENT_WILL_MSG.length();
        mqtt_cfg.session.last_will.qos = 1;
        mqtt_cfg.session.last_will.retain = true;
        mqtt_cfg.network.disable_auto_reconnect = true;
        mMqttClientHandle = esp_mqtt_client_init(&mqtt_cfg);
        if (mMqttClientHandle == NULL)
        {
            ESP_LOGE(TAG, "Failed to create MQTT client!");
            return ESP_FAIL;
        }
        // Create one-shot reconnect timer (fires when broker drops but WiFi is still up)
        esp_timer_create_args_t timer_args = {};
        timer_args.callback = mqtt_reconnect_timer_cb;
        timer_args.arg = this;
        timer_args.name = "mqtt_reconnect";
        if (esp_timer_create(&timer_args, &mReconnectTimer) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create MQTT reconnect timer!");
            esp_mqtt_client_destroy(mMqttClientHandle);
            mMqttClientHandle = nullptr;
            return ESP_FAIL;
        }
        // Register event handler
        err = esp_mqtt_client_register_event(mMqttClientHandle, MQTT_EVENT_ANY, mqtt_event_handler, this);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to register MQTT event handler! (%d)", err);
            esp_timer_delete(mReconnectTimer);
            mReconnectTimer = nullptr;
            esp_mqtt_client_destroy(mMqttClientHandle);
            mMqttClientHandle = nullptr;
            return ESP_FAIL;
        }
        // Register network event handlers to coordinate MQTT reconnect with WiFi/Ethernet state
#ifdef CONFIG_CONNECTIVITY_CHOICE_WIFI
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &mqtt_network_event_handler, this);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &mqtt_network_event_handler, this);
#endif
#ifdef CONFIG_CONNECTIVITY_CHOICE_ETH
        esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &mqtt_network_event_handler, this);
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &mqtt_network_event_handler, this);
#endif
        // Start
        err = esp_mqtt_client_start(mMqttClientHandle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to start MQTT client! (%d)", err);
            esp_timer_delete(mReconnectTimer);
            mReconnectTimer = nullptr;
            esp_mqtt_client_destroy(mMqttClientHandle);
            mMqttClientHandle = nullptr;
            return ESP_FAIL;
        }
        mStarted = true;
        return err;
    }
    void MqttHelpers::OnNetworkConnected()
    {
        if (!mStarted || mMqttClientHandle == nullptr)
            return;
        esp_timer_stop(mReconnectTimer); // cancel any pending broker-drop retry
        ESP_LOGI(TAG, "Network up — triggering MQTT reconnect");
        esp_mqtt_client_reconnect(mMqttClientHandle);
    }
    void MqttHelpers::OnNetworkDisconnected()
    {
        if (!mStarted || mReconnectTimer == nullptr)
            return;
        ESP_LOGI(TAG, "Network down — cancelling MQTT reconnect timer");
        esp_timer_stop(mReconnectTimer);
    }
    void MqttHelpers::OnMqttDisconnected()
    {
        if (!mStarted || mMqttClientHandle == nullptr || mReconnectTimer == nullptr)
            return;
        if (NetworkHelpers::isConnected())
        {
            // WiFi is up — broker dropped independently; retry in 5 seconds
            esp_timer_stop(mReconnectTimer);
            esp_timer_start_once(mReconnectTimer, 5ULL * 1000 * 1000);
            ESP_LOGI(TAG, "Broker unreachable — will retry in 5s");
        }
        // If WiFi is down, OnNetworkConnected() will trigger reconnect when IP is obtained
    }
    void MqttHelpers::SendDiscovery()
    {
        // See https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
        bool error = false;
        std::string discoveryId = MqttConfig::GetDiscoveryIdName();
        std::string availability_topic = mTopicPrefix + MQTT_CLIENT_BIRTH_WILL_TOPIC;
        const esp_app_desc_t *desc = esp_app_get_description();

        cJSON *discovery = cJSON_CreateObject();
        if (discovery == NULL)
            return;

        // "dev" section — the controller
        cJSON *dev = cJSON_AddObjectToObject(discovery, "dev");
        if (dev == NULL)
            error = true;
        else
        {
            error = error || (cJSON_AddStringToObject(dev, "ids", discoveryId.c_str()) == NULL);  // identifiers
            error = error || (cJSON_AddStringToObject(dev, "name", discoveryId.c_str()) == NULL); // name
            error = error || (cJSON_AddStringToObject(dev, "mf", "nicolas5000") == NULL);         // manufacturer
            error = error || (cJSON_AddStringToObject(dev, "sw", desc->version) == NULL);         // sw_version
        }

        // "o" (origin) section
        if (!error)
        {
            cJSON *o = cJSON_AddObjectToObject(discovery, "o");
            if (o == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(o, "name", discoveryId.c_str()) == NULL);                           // name
                error = error || (cJSON_AddStringToObject(o, "url", "https://github.com/nicolas5000/diagral-esp32") == NULL); // support_url
                error = error || (cJSON_AddStringToObject(o, "sw", desc->version) == NULL);                                   // sw_version
            }
        }

        // Shared availability
        error = error || (cJSON_AddStringToObject(discovery, "availability_topic", availability_topic.c_str()) == NULL);

        // "cmps" (components) section
        cJSON *cmps = NULL;
        if (!error)
        {
            cmps = cJSON_AddObjectToObject(discovery, "cmps");
            if (cmps == NULL)
                error = true;
        }
        if (!error)
        {
            // Add reboot button https://www.home-assistant.io/integrations/button.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "reboot");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "button") == NULL); // platform
                std::string unique_id = discoveryId + "_" + MQTT_CLIENT_REBOOT_ID;
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Reboot") == NULL);               // name
                std::string reboot_topic = mTopicPrefix + "/" + MQTT_CLIENT_REBOOT_ID + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", reboot_topic.c_str()) == NULL); // command_topic
            }
        }
        if (!error)
        {
            // Add "Logging" switch https://www.home-assistant.io/integrations/switch.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, MQTT_CLIENT_LOGGING_ID.c_str());
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "switch") == NULL); // platform
                std::string unique_id = discoveryId + "_" + MQTT_CLIENT_LOGGING_ID;
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Enable logging") == NULL);       // name
                error = error || (cJSON_AddBoolToObject(cmp, "optimistic", true) == NULL);               // optimistic
                std::string state_topic = mTopicPrefix + "/" + MQTT_CLIENT_CONFIG_ID + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL); // state_topic
                std::string value_template = "{{ value_json." + MQTT_CLIENT_LOGGING_ID + " }}";
                error = error || (cJSON_AddStringToObject(cmp, "value_template", value_template.c_str()) == NULL); // value_template
                std::string command_topic = mTopicPrefix + "/" + MQTT_CLIENT_CONFIG_ID + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_LOGGING_ID + "\": \"{{ value }}\"}";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
            }
        }
        if (!error)
        {
            // Add "Passive" switch https://www.home-assistant.io/integrations/switch.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, MQTT_CLIENT_PASSIVE_ID.c_str());
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "switch") == NULL); // platform
                std::string unique_id = discoveryId + "_" + MQTT_CLIENT_PASSIVE_ID;
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Enable Passive mode") == NULL);  // name
                error = error || (cJSON_AddBoolToObject(cmp, "optimistic", true) == NULL);               // optimistic
                std::string state_topic = mTopicPrefix + "/" + MQTT_CLIENT_CONFIG_ID + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL); // state_topic
                std::string value_template = "{{ value_json." + MQTT_CLIENT_PASSIVE_ID + " }}";
                error = error || (cJSON_AddStringToObject(cmp, "value_template", value_template.c_str()) == NULL); // value_template
                std::string command_topic = mTopicPrefix + "/" + MQTT_CLIENT_CONFIG_ID + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_PASSIVE_ID + "\": \"{{ value }}\"}";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
            }
        }
        if (!error)
        {
            // Add 'Mode' sensor
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "mode");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "sensor") == NULL); // platform
                std::string unique_id = discoveryId + "_mode";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL);  // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Mode") == NULL);                  // name
                error = error || (cJSON_AddStringToObject(cmp, "entity_category", "diagnostic") == NULL); // entity_category
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_INFO_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);        // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.mode }}") == NULL); // value_template
            }
        }
        if (!error)
        {
            // Add 'Power supply' sensor
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "power_supply");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "sensor") == NULL); // platform
                std::string unique_id = discoveryId + "_power_supply";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL);  // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Power supply") == NULL);          // name
                error = error || (cJSON_AddStringToObject(cmp, "entity_category", "diagnostic") == NULL); // entity_category
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_INFO_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);                // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.power_supply }}") == NULL); // value_template
            }
        }
        if (!error)
        {
            // Add 'Battery' sensor
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "battery");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "sensor") == NULL); // platform
                std::string unique_id = discoveryId + "_battery";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL);  // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Battery") == NULL);               // name
                error = error || (cJSON_AddStringToObject(cmp, "entity_category", "diagnostic") == NULL); // entity_category
                error = error || (cJSON_AddStringToObject(cmp, "device_class", "battery") == NULL);       // device_class
                error = error || (cJSON_AddStringToObject(cmp, "unit_of_measurement", "%") == NULL);      // unit_of_measurement
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_INFO_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);           // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.battery }}") == NULL); // value_template
            }
        }
        if (!error)
        {
            // Add 'Control panel' alarm_control_panel https://www.home-assistant.io/integrations/alarm_control_panel.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "panel");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "alarm_control_panel") == NULL); // platform
                std::string unique_id = discoveryId + "_alarm_control_panel";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Control panel") == NULL);        // name
                std::string command_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_CONTROL_PANEL_ACTION_ID + "\": \"{{ action }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_CODE_ID + "\": \"{{ code }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_ZONES_ID + "\": 15 }";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);       // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.all }}") == NULL); // value_template
                error = error || (cJSON_AddBoolToObject(cmp, "code_arm_required", mIsPinCheckEnabled) == NULL);    // code_arm_required
                error = error || (cJSON_AddBoolToObject(cmp, "code_disarm_required", mIsPinCheckEnabled) == NULL); // code_disarm_required
                if (mIsPinCheckEnabled)
                {
                    error = error || (cJSON_AddStringToObject(cmp, "code", "REMOTE_CODE") == NULL); // code
                }
                cJSON *supported = cJSON_AddArrayToObject(cmp, "supported_features"); // supported_features
                if (supported != NULL)
                {
                    cJSON_AddItemToArray(supported, cJSON_CreateString("arm_home"));
                    cJSON_AddItemToArray(supported, cJSON_CreateString("arm_away"));
                }
                else
                    error = true;
            }
        }
        if (!error)
        {
            // Add 'Control panel zone 1' alarm_control_panel https://www.home-assistant.io/integrations/alarm_control_panel.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "zone1");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "alarm_control_panel") == NULL); // platform
                std::string unique_id = discoveryId + "_alarm_control_panel_zone1";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Control panel zone 1") == NULL); // name
                std::string command_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_CONTROL_PANEL_ACTION_ID + "\": \"{{ action }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_CODE_ID + "\": \"{{ code }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_ZONES_ID + "\": 1 }";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);         // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.zone1 }}") == NULL); // value_template
                error = error || (cJSON_AddBoolToObject(cmp, "code_arm_required", mIsPinCheckEnabled) == NULL);      // code_arm_required
                error = error || (cJSON_AddBoolToObject(cmp, "code_disarm_required", mIsPinCheckEnabled) == NULL);   // code_disarm_required
                if (mIsPinCheckEnabled)
                {
                    error = error || (cJSON_AddStringToObject(cmp, "code", "REMOTE_CODE") == NULL); // code
                }
                cJSON *supported = cJSON_AddArrayToObject(cmp, "supported_features"); // supported_features
                if (supported != NULL)
                {
                    cJSON_AddItemToArray(supported, cJSON_CreateString("arm_away"));
                }
                else
                    error = true;
            }
        }
        if (!error)
        {
            // Add 'Control panel zone 2' alarm_control_panel https://www.home-assistant.io/integrations/alarm_control_panel.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "zone2");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "alarm_control_panel") == NULL); // platform
                std::string unique_id = discoveryId + "_alarm_control_panel_zone2";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Control panel zone 2") == NULL); // name
                std::string command_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_CONTROL_PANEL_ACTION_ID + "\": \"{{ action }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_CODE_ID + "\": \"{{ code }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_ZONES_ID + "\": 2 }";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);         // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.zone2 }}") == NULL); // value_template
                error = error || (cJSON_AddBoolToObject(cmp, "code_arm_required", mIsPinCheckEnabled) == NULL);      // code_arm_required
                error = error || (cJSON_AddBoolToObject(cmp, "code_disarm_required", mIsPinCheckEnabled) == NULL);   // code_disarm_required
                if (mIsPinCheckEnabled)
                {
                    error = error || (cJSON_AddStringToObject(cmp, "code", "REMOTE_CODE") == NULL); // code
                }
                cJSON *supported = cJSON_AddArrayToObject(cmp, "supported_features"); // supported_features
                if (supported != NULL)
                {
                    cJSON_AddItemToArray(supported, cJSON_CreateString("arm_away"));
                }
                else
                    error = true;
            }
        }
        if (!error)
        {
            // Add 'Control panel zone 3' alarm_control_panel https://www.home-assistant.io/integrations/alarm_control_panel.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "zone3");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "alarm_control_panel") == NULL); // platform
                std::string unique_id = discoveryId + "_alarm_control_panel_zone3";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Control panel zone 3") == NULL); // name
                std::string command_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_CONTROL_PANEL_ACTION_ID + "\": \"{{ action }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_CODE_ID + "\": \"{{ code }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_ZONES_ID + "\": 4 }";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);         // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.zone3 }}") == NULL); // value_template
                error = error || (cJSON_AddBoolToObject(cmp, "code_arm_required", mIsPinCheckEnabled) == NULL);      // code_arm_required
                error = error || (cJSON_AddBoolToObject(cmp, "code_disarm_required", mIsPinCheckEnabled) == NULL);   // code_disarm_required
                if (mIsPinCheckEnabled)
                {
                    error = error || (cJSON_AddStringToObject(cmp, "code", "REMOTE_CODE") == NULL); // code
                }
                cJSON *supported = cJSON_AddArrayToObject(cmp, "supported_features"); // supported_features
                if (supported != NULL)
                {
                    cJSON_AddItemToArray(supported, cJSON_CreateString("arm_away"));
                }
                else
                    error = true;
            }
        }
        if (!error)
        {
            // Add 'Control panel zone 4' alarm_control_panel https://www.home-assistant.io/integrations/alarm_control_panel.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, "zone4");
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "alarm_control_panel") == NULL); // platform
                std::string unique_id = discoveryId + "_alarm_control_panel_zone4";
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Control panel zone 4") == NULL); // name
                std::string command_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_COMMAND_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "command_topic", command_topic.c_str()) == NULL); // command_topic
                std::string command_template = "{\"" + MQTT_CLIENT_CONTROL_PANEL_ACTION_ID + "\": \"{{ action }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_CODE_ID + "\": \"{{ code }}\", \"" + MQTT_CLIENT_CONTROL_PANEL_ZONES_ID + "\": 8 }";
                error = error || (cJSON_AddStringToObject(cmp, "command_template", command_template.c_str()) == NULL); // command_template
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_STATE_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL);         // state_topic
                error = error || (cJSON_AddStringToObject(cmp, "value_template", "{{ value_json.zone4 }}") == NULL); // value_template
                error = error || (cJSON_AddBoolToObject(cmp, "code_arm_required", mIsPinCheckEnabled) == NULL);      // code_arm_required
                error = error || (cJSON_AddBoolToObject(cmp, "code_disarm_required", mIsPinCheckEnabled) == NULL);   // code_disarm_required
                if (mIsPinCheckEnabled)
                {
                    error = error || (cJSON_AddStringToObject(cmp, "code", "REMOTE_CODE") == NULL); // code
                }
                cJSON *supported = cJSON_AddArrayToObject(cmp, "supported_features"); // supported_features
                if (supported != NULL)
                {
                    cJSON_AddItemToArray(supported, cJSON_CreateString("arm_away"));
                }
                else
                    error = true;
            }
        }
        if (!error)
        {
            // Add 'Last detection' event https://www.home-assistant.io/integrations/event.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, MQTT_CLIENT_LAST_DETECTION_ID.c_str());
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "event") == NULL); // platform
                std::string unique_id = discoveryId + "_" + MQTT_CLIENT_LAST_DETECTION_ID;
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Last detection") == NULL);       // name
                cJSON *events = cJSON_AddArrayToObject(cmp, "event_types");                              // event_types
                if (events != NULL)
                {
                    cJSON_AddItemToArray(events, cJSON_CreateString("dissuasion"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("pre_alarm"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("caution"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("intrusion"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("timer_start"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("timer_end"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("pre_alarm_confirmed"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("intrusion_confirmed"));
                }
                else
                    error = true;
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_LAST_DETECTION_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL); // state_topic
            }
        }
        if (!error)
        {
            // Add 'Last alert' event https://www.home-assistant.io/integrations/event.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, MQTT_CLIENT_LAST_ALERT_ID.c_str());
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "event") == NULL); // platform
                std::string unique_id = discoveryId + "_" + MQTT_CLIENT_LAST_ALERT_ID;
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Last alert") == NULL);           // name
                cJSON *events = cJSON_AddArrayToObject(cmp, "event_types");                              // event_types
                if (events != NULL)
                {
                    cJSON_AddItemToArray(events, cJSON_CreateString("alert"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("fire"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("silent"));
                }
                else
                    error = true;
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_LAST_ALERT_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL); // state_topic
            }
        }
        if (!error)
        {
            // Add 'Last tamper' event https://www.home-assistant.io/integrations/event.mqtt/
            cJSON *cmp = cJSON_AddObjectToObject(cmps, MQTT_CLIENT_LAST_TAMPER_ID.c_str());
            if (cmp == NULL)
                error = true;
            else
            {
                error = error || (cJSON_AddStringToObject(cmp, "p", "event") == NULL); // platform
                std::string unique_id = discoveryId + "_" + MQTT_CLIENT_LAST_TAMPER_ID;
                error = error || (cJSON_AddStringToObject(cmp, "unique_id", unique_id.c_str()) == NULL); // unique_id
                error = error || (cJSON_AddStringToObject(cmp, "name", "Last tamper") == NULL);          // name
                cJSON *events = cJSON_AddArrayToObject(cmp, "event_types");                              // event_types
                if (events != NULL)
                {
                    cJSON_AddItemToArray(events, cJSON_CreateString("tamper"));
                    cJSON_AddItemToArray(events, cJSON_CreateString("clear"));
                }
                else
                    error = true;
                std::string state_topic = mTopicPrefix + MQTT_CLIENT_LAST_TAMPER_TOPIC;
                error = error || (cJSON_AddStringToObject(cmp, "state_topic", state_topic.c_str()) == NULL); // state_topic
            }
        }

        // Publish controller discovery
        if (!error)
        {
            std::string topic = mDiscoveryPrefix + MQTT_CLIENT_DISCOVERY_TOPIC;
            const char *data = cJSON_Print(discovery);
            if (data == NULL)
            {
                ESP_LOGE(TAG, "Failed to create controller discovery string");
            }
            else
            {
                esp_mqtt_client_publish(mMqttClientHandle, topic.c_str(), data, 0, 0, 1);
                cJSON_free((void *)data);
                ESP_LOGI(TAG, "Sent controller discovery successfully");
            }
        }
        cJSON_Delete(discovery);
    }
    void MqttHelpers::UpdateAndSendDeviceState(const Diagral::DiagralDeviceState &state)
    {
        if (!mStarted || mMqttClientHandle == nullptr)
            return;

        // send device info if updated
        bool send_info = (sDiagralDeviceState.mode != state.mode) || (sDiagralDeviceState.power != state.power) || (sDiagralDeviceState.battery != state.battery);

        // send device state if updated
        bool send_state = sDiagralDeviceState.lastStateTimestamp != state.lastStateTimestamp;

        // send last detection if updated
        bool send_detection = sDiagralDeviceState.lastDetection.timestamp != state.lastDetection.timestamp;

        // send last alert if updated
        bool send_alert = sDiagralDeviceState.lastAlert.timestamp != state.lastAlert.timestamp;

        // send last tamper if updated
        bool send_tamper = sDiagralDeviceState.lastTamper.timestamp != state.lastTamper.timestamp;

        // update device local state
        memcpy((void *)&sDiagralDeviceState, (void *)&state, sizeof(state));

        // send MQTT messages
        SendDeviceState(send_info, send_state, send_detection, send_alert, send_tamper);
    }
    void MqttHelpers::SendDeviceState(bool info, bool panel, bool detection, bool alert, bool tamper)
    {
        if (info && sDiagralDeviceState.lastStateTimestamp != 0)
        {
            // send device info
            cJSON *infoData = cJSON_CreateObject();
            if (infoData != NULL)
            {
                cJSON_AddStringToObject(infoData, "mode", DiagralModeToString(sDiagralDeviceState.mode).c_str());
                cJSON_AddStringToObject(infoData, "power_supply", DiagralPowerSupplyToString(sDiagralDeviceState.power).c_str());
                cJSON_AddNumberToObject(infoData, "battery", sDiagralDeviceState.battery);
                const char *data = cJSON_Print(infoData);
                if (data == NULL)
                {
                    ESP_LOGE(TAG, "Failed to create device info string");
                }
                else
                {
                    std::string infoTopic = GetTopicPrefix() + MQTT_CLIENT_INFO_TOPIC;
                    esp_mqtt_client_publish(mMqttClientHandle, infoTopic.c_str(), data, 0, 0, 1);
                    cJSON_free((void *)data);
                    ESP_LOGI(TAG, "Sent device info successfully");
                }
                cJSON_Delete(infoData);
            }
        }

        if (panel && sDiagralDeviceState.lastStateTimestamp != 0)
        {
            // send device state (alarm panels)
            cJSON *stateData = cJSON_CreateObject();
            if (stateData != NULL)
            {
                DiagralState allZones = DiagralState::DIAGRAL_STATE_DISARMED;
                if (sDiagralDeviceState.zone1 == DiagralState::DIAGRAL_STATE_TRIGGERED || sDiagralDeviceState.zone2 == DiagralState::DIAGRAL_STATE_TRIGGERED || sDiagralDeviceState.zone3 == DiagralState::DIAGRAL_STATE_TRIGGERED || sDiagralDeviceState.zone4 == DiagralState::DIAGRAL_STATE_TRIGGERED)
                    allZones = DiagralState::DIAGRAL_STATE_TRIGGERED;
                else if (sDiagralDeviceState.zone1 == DiagralState::DIAGRAL_STATE_ARMED || sDiagralDeviceState.zone2 == DiagralState::DIAGRAL_STATE_ARMED || sDiagralDeviceState.zone3 == DiagralState::DIAGRAL_STATE_ARMED || sDiagralDeviceState.zone4 == DiagralState::DIAGRAL_STATE_ARMED)
                    allZones = DiagralState::DIAGRAL_STATE_ARMED;
                else if (sDiagralDeviceState.zone1 == DiagralState::DIAGRAL_STATE_ARMED_HOME || sDiagralDeviceState.zone2 == DiagralState::DIAGRAL_STATE_ARMED_HOME || sDiagralDeviceState.zone3 == DiagralState::DIAGRAL_STATE_ARMED_HOME || sDiagralDeviceState.zone4 == DiagralState::DIAGRAL_STATE_ARMED_HOME)
                    allZones = DiagralState::DIAGRAL_STATE_ARMED_HOME;
                else if (sDiagralDeviceState.zone1 == DiagralState::DIAGRAL_STATE_ARMING || sDiagralDeviceState.zone2 == DiagralState::DIAGRAL_STATE_ARMING || sDiagralDeviceState.zone3 == DiagralState::DIAGRAL_STATE_ARMING || sDiagralDeviceState.zone4 == DiagralState::DIAGRAL_STATE_ARMING)
                    allZones = DiagralState::DIAGRAL_STATE_ARMING;
                cJSON_AddStringToObject(stateData, "all", DiagralStateToString(allZones).c_str());
                cJSON_AddStringToObject(stateData, "zone1", DiagralStateToString(sDiagralDeviceState.zone1).c_str());
                cJSON_AddStringToObject(stateData, "zone2", DiagralStateToString(sDiagralDeviceState.zone2).c_str());
                cJSON_AddStringToObject(stateData, "zone3", DiagralStateToString(sDiagralDeviceState.zone3).c_str());
                cJSON_AddStringToObject(stateData, "zone4", DiagralStateToString(sDiagralDeviceState.zone4).c_str());
                char strftime_buf[64];
                struct tm timeinfo;
                time_t current_time;
                time(&current_time);
                int64_t time_offset = esp_timer_get_time() - sDiagralDeviceState.lastStateTimestamp;
                if (time_offset < 0)
                    time_offset += INT64_MAX; // esp_timer reset
                current_time -= time_offset / 1000000; // us to s!
                localtime_r(&current_time, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%FT%TZ", &timeinfo);
                cJSON_AddStringToObject(stateData, "timestamp", strftime_buf);
                const char *data = cJSON_Print(stateData);
                if (data == NULL)
                {
                    ESP_LOGE(TAG, "Failed to create device state string");
                }
                else
                {
                    std::string stateTopic = GetTopicPrefix() + MQTT_CLIENT_CONTROL_PANEL_TOPIC + MQTT_CLIENT_STATE_TOPIC;
                    esp_mqtt_client_publish(mMqttClientHandle, stateTopic.c_str(), data, 0, 0, 1);
                    cJSON_free((void *)data);
                    ESP_LOGI(TAG, "Sent device state successfully");
                }
                cJSON_Delete(stateData);
            }
        }

        if (detection && sDiagralDeviceState.lastDetection.timestamp != 0)
        {
            // send last detection
            cJSON *detectionData = cJSON_CreateObject();
            if (detectionData != NULL)
            {
                cJSON_AddStringToObject(detectionData, "event_type", DiagralDetectionEventTypeToString(sDiagralDeviceState.lastDetection.eventType).c_str());
                cJSON_AddStringToObject(detectionData, "sensor_type", DiagralSensorTypeToString(sDiagralDeviceState.lastDetection.sensorType).c_str());
                cJSON_AddNumberToObject(detectionData, "sensor_num", sDiagralDeviceState.lastDetection.sensorNumber);
                char strftime_buf[64];
                struct tm timeinfo;
                localtime_r(&sDiagralDeviceState.lastDetection.timestamp, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%FT%TZ", &timeinfo);
                cJSON_AddStringToObject(detectionData, "timestamp", strftime_buf);
                const char *data = cJSON_Print(detectionData);
                if (data == NULL)
                {
                    ESP_LOGE(TAG, "Failed to create detection string");
                }
                else
                {
                    std::string detectionTopic = GetTopicPrefix() + MQTT_CLIENT_LAST_DETECTION_TOPIC;
                    esp_mqtt_client_publish(mMqttClientHandle, detectionTopic.c_str(), data, 0, 0, 1);
                    cJSON_free((void *)data);
                    ESP_LOGI(TAG, "Sent detection successfully");
                }
                cJSON_Delete(detectionData);
            }
        }

        if (alert && sDiagralDeviceState.lastAlert.timestamp != 0)
        {
            // send last alert
            cJSON *alertData = cJSON_CreateObject();
            if (alertData != NULL)
            {
                cJSON_AddStringToObject(alertData, "event_type", DiagralAlertTypeToString(sDiagralDeviceState.lastAlert.type).c_str());
                cJSON_AddNumberToObject(alertData, "command_num", sDiagralDeviceState.lastAlert.commandNumber);
                char strftime_buf[64];
                struct tm timeinfo;
                localtime_r(&sDiagralDeviceState.lastAlert.timestamp, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%FT%TZ", &timeinfo);
                cJSON_AddStringToObject(alertData, "timestamp", strftime_buf);
                const char *data = cJSON_Print(alertData);
                if (data == NULL)
                {
                    ESP_LOGE(TAG, "Failed to create alert string");
                }
                else
                {
                    std::string alertTopic = GetTopicPrefix() + MQTT_CLIENT_LAST_ALERT_TOPIC;
                    esp_mqtt_client_publish(mMqttClientHandle, alertTopic.c_str(), data, 0, 0, 1);
                    cJSON_free((void *)data);
                    ESP_LOGI(TAG, "Sent alert successfully");
                }
                cJSON_Delete(alertData);
            }
        }

        if (tamper && sDiagralDeviceState.lastTamper.timestamp != 0)
        {
            // send last tamper
            cJSON *tamperData = cJSON_CreateObject();
            if (tamperData != NULL)
            {
                cJSON_AddStringToObject(tamperData, "event_type", sDiagralDeviceState.lastTamper.isActive ? "tamper" : "clear");
                cJSON_AddNumberToObject(tamperData, "sensor_num", sDiagralDeviceState.lastTamper.sensorNumber);
                char strftime_buf[64];
                struct tm timeinfo;
                localtime_r(&sDiagralDeviceState.lastTamper.timestamp, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%FT%TZ", &timeinfo);
                cJSON_AddStringToObject(tamperData, "timestamp", strftime_buf);
                const char *data = cJSON_Print(tamperData);
                if (data == NULL)
                {
                    ESP_LOGE(TAG, "Failed to create tamper string");
                }
                else
                {
                    std::string tamperTopic = GetTopicPrefix() + MQTT_CLIENT_LAST_TAMPER_TOPIC;
                    esp_mqtt_client_publish(mMqttClientHandle, tamperTopic.c_str(), data, 0, 0, 1);
                    cJSON_free((void *)data);
                    ESP_LOGI(TAG, "Sent tamper successfully");
                }
                cJSON_Delete(tamperData);
            }
        }
    }
}