# MQTT interface

## Configuration

See [Project configuration](configuration.md#mqtt-configuration-parameters) and [Command line configuration](command_line.md#configure-mqtt-client) to configure MQTT client, discovery prefix and topic prefix.

## Discovery

Discovery is compatible with Home Assistant discovery, that automatically adds devices to Home Assistant without any manual configuration from user.
See [Home Assistant MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) for more details.

### Controller discovery

Main features provided by the ESP32 are exposed to _MQTT_DISCOVERY_PREFIX/config_
By default, _MQTT_DISCOVERY_PREFIX_ is equal to _homeassistant/device/diagral-uart_, so complete path is _homeassistant/device/diagral-uart/config_

Discovery message exposes controller description and mandatory fields required by Home Assistant. In addition, it also exposes some features and configuration parameters like:
- Diagral logging configuration (change is applied after reboot)
- Diagral passive mode configuration (change is applied after reboot)
- _Reboot_ button: this button orders an immediate soft reboot of the controller
- _OTA Upgrade URL_ text: stores the firmware URL without starting an update
- _OTA Upgrade_ button: starts an update using the saved URL
- _OTA Status_ sensor: reports update, validation and failure events

Here is an example of discovery message sent to this topic:
```json
{
	"dev":	{
		"ids":	"diagral-esp32",
		"name":	"diagral-esp32",
		"mf":	"nicolas5000",
		"sw":	"1"
	},
	"o":	{
		"name":	"diagral-esp32",
		"url":	"https://github.com/nicolas5000/diagral-esp32",
		"sw":	"1"
	},
	"availability": [{"topic": "diagral-esp32/status"}],
	"cmps":	{
		"reboot":	{
			"p":	"button",
			"unique_id":	"diagral-esp32_button_reboot",
			"name":	"Reboot",
			"command_topic":	"diagral-esp32/button_reboot/set"
		},
		"upgrade":	{
			"p":	"text",
			"unique_id":	"diagral-esp32_ota_upgrade",
			"name":	"OTA Upgrade URL",
			"entity_category":	"config",
			"visible_by_default":	false,
			"command_topic":	"diagral-esp32/ota_upgrade/set",
			"state_topic": "diagral-esp32/ota_upgrade/state",
			"min": 0,
			"max": 255,
			"retain": false,
			"command_template": "{\"url\": {{ value | to_json }}}"
		},
		"ota_button": {
			"p": "button",
			"unique_id": "diagral-esp32_button_ota_upgrade",
			"name": "OTA Upgrade",
			"entity_category": "config",
			"command_topic": "diagral-esp32/button_ota_upgrade/set",
			"payload_press": "PRESS",
			"retain": false,
			"availability": [
				{"topic": "diagral-esp32/status"},
				{
					"topic": "diagral-esp32/ota_upgrade/state",
					"value_template": "{{ 'online' if value | length > 0 else 'offline' }}"
				}
			],
			"availability_mode": "all"
		},
		"ota_status": {
			"p": "sensor",
			"unique_id": "diagral-esp32_ota_status",
			"name": "OTA Status",
			"entity_category": "diagnostic",
			"state_topic": "diagral-esp32/ota_status/state",
			"value_template": "{{ value_json.status }}",
			"json_attributes_topic": "diagral-esp32/ota_status/state"
		},
		"Logging":	{
			"p":	"switch",
			"unique_id":	"diagral-esp32_Logging",
			"name":	"Enable logging",
			"optimistic":	true,
			"state_topic":	"diagral-esp32/config/state",
			"value_template":	"{{ value_json.Logging }}",
			"command_topic":	"diagral-esp32/config/set",
			"command_template":	"{\"Logging\": \"{{ value }}\"}"
		},
		"Passive":	{
			"p":	"switch",
			"unique_id":	"diagral-esp32_Passive",
			"name":	"Enable Passive mode",
			"optimistic":	true,
			"state_topic":	"diagral-esp32/config/state",
			"value_template":	"{{ value_json.Passive }}",
			"command_topic":	"diagral-esp32/config/set",
			"command_template":	"{\"Passive\": \"{{ value }}\"}"
		},
		"error":	{
			"p":	"binary_sensor",
			"unique_id":	"diagral-esp32_error",
			"name":	"Error state",
			"entity_category":	"diagnostic",
			"device_class":	"problem",
			"state_topic":	"diagral-esp32/info",
			"value_template":	"{{ value_json.error }}"
		},
		"mode":	{
			"p":	"sensor",
			"unique_id":	"diagral-esp32_mode",
			"name":	"Mode",
			"entity_category":	"diagnostic",
			"state_topic":	"diagral-esp32/info",
			"value_template":	"{{ value_json.mode }}"
		},
		"battery":	{
			"p":	"sensor",
			"unique_id":	"diagral-esp32_battery",
			"name":	"Battery",
			"entity_category":	"diagnostic",
			"device_class":	"battery",
			"unit_of_measurement":	"%",
			"state_topic":	"diagral-esp32/info",
			"value_template":	"{{ value_json.battery }}"
		},
		"panel":	{
			"p":	"alarm_control_panel",
			"unique_id":	"diagral-esp32_alarm_control_panel",
			"name":	"Control panel",
			"command_topic":	"diagral-esp32/control_panel/set",
			"command_template":	"{\"action\": \"{{ action }}\", \"code\": \"{{ code }}\", \"zones\": 15 }",
			"state_topic":	"diagral-esp32/control_panel/state",
			"value_template":	"{{ value_json.all }}",
			"code_arm_required":	true,
			"code_disarm_required":	true,
			"code":	"REMOTE_CODE",
			"supported_features":	["arm_home", "arm_away"]
		},
		"zone1":	{
			"p":	"alarm_control_panel",
			"unique_id":	"diagral-esp32_alarm_control_panel_zone1",
			"name":	"Control panel zone 1",
			"command_topic":	"diagral-esp32/control_panel/set",
			"command_template":	"{\"action\": \"{{ action }}\", \"code\": \"{{ code }}\", \"zones\": 1 }",
			"state_topic":	"diagral-esp32/control_panel/state",
			"value_template":	"{{ value_json.zone1 }}",
			"code_arm_required":	true,
			"code_disarm_required":	true,
			"code":	"REMOTE_CODE",
			"supported_features":	["arm_away"]
		},
		"zone2":	{
			"p":	"alarm_control_panel",
			"unique_id":	"diagral-esp32_alarm_control_panel_zone2",
			"name":	"Control panel zone 2",
			"command_topic":	"diagral-esp32/control_panel/set",
			"command_template":	"{\"action\": \"{{ action }}\", \"code\": \"{{ code }}\", \"zones\": 2 }",
			"state_topic":	"diagral-esp32/control_panel/state",
			"value_template":	"{{ value_json.zone2 }}",
			"code_arm_required":	true,
			"code_disarm_required":	true,
			"code":	"REMOTE_CODE",
			"supported_features":	["arm_away"]
		},
		"zone3":	{
			"p":	"alarm_control_panel",
			"unique_id":	"diagral-esp32_alarm_control_panel_zone3",
			"name":	"Control panel zone 3",
			"command_topic":	"diagral-esp32/control_panel/set",
			"command_template":	"{\"action\": \"{{ action }}\", \"code\": \"{{ code }}\", \"zones\": 4 }",
			"state_topic":	"diagral-esp32/control_panel/state",
			"value_template":	"{{ value_json.zone3 }}",
			"code_arm_required":	true,
			"code_disarm_required":	true,
			"code":	"REMOTE_CODE",
			"supported_features":	["arm_away"]
		},
		"zone4":	{
			"p":	"alarm_control_panel",
			"unique_id":	"diagral-esp32_alarm_control_panel_zone4",
			"name":	"Control panel zone 4",
			"command_topic":	"diagral-esp32/control_panel/set",
			"command_template":	"{\"action\": \"{{ action }}\", \"code\": \"{{ code }}\", \"zones\": 8 }",
			"state_topic":	"diagral-esp32/control_panel/state",
			"value_template":	"{{ value_json.zone4 }}",
			"code_arm_required":	true,
			"code_disarm_required":	true,
			"code":	"REMOTE_CODE",
			"supported_features":	["arm_away"]
		},
		"last_detection":	{
			"p":	"event",
			"unique_id":	"diagral-esp32_last_detection",
			"name":	"Last detection",
			"event_types":	["dissuasion", "pre_alarm", "caution", "intrusion", "timer_start", "timer_end", "pre_alarm_confirmed", "intrusion_confirmed"],
			"state_topic":	"diagral-esp32/last_detection"
		},
		"last_alert":	{
			"p":	"event",
			"unique_id":	"diagral-esp32_last_alert",
			"name":	"Last alert",
			"event_types":	["alert", "fire", "silent", "duress_disarm"],
			"state_topic":	"diagral-esp32/last_alert"
		},
		"last_error":	{
			"p":	"event",
			"unique_id":	"diagral-esp32_last_error",
			"name":	"Last error",
			"event_types":	["tamper", "tamper_clear", "main_power_lost", "main_power_restored", "battery_low", "battery_restored", "radio_lost", "radio_restored", "link_lost", "link_restored", "arm_cancel_issue_open", "arm_cancel_transmitter_error", "arm_cancel_tamper"],
			"state_topic":	"diagral-esp32/last_error"
		}
	}
}

```

## Topics and payloads

All topics are prefixed by _MQTT_TOPIC_PREFIX_, that is by default _diagral-uart_  
All topics and payloads are described in discovery messages and compatible with Home Assistant, so they will not be described here. See [Home Assistant MQTT documentation](https://www.home-assistant.io/integrations/mqtt/) for more details.

### OTA workflow

The OTA URL command has changed behavior: sending a URL only saves it. Existing MQTT publishers must send a separate button command to start an update. Use the configured _MQTT_TOPIC_PREFIX_ in place of `<prefix>` below.

1. Publish `{"url":"https://firmware.example.test/diagral-uart-esp32.bin"}` to `<prefix>/ota_upgrade/set`, without retain. This saves the URL in the dedicated `ota` NVS namespace and publishes its retained plain-text value on `<prefix>/ota_upgrade/state`.
2. Publish `PRESS` to `<prefix>/button_ota_upgrade/set`, without retain, to start the update. Concurrent update requests are rejected. The Home Assistant button is available only while the device is online and the stored URL is nonempty; availability does not check the server itself.
3. Observe `<prefix>/ota_status/state`. The retained JSON payload contains `status` and `detail`; ESP-IDF failures include an error name in `detail`.

URLs must end in `.bin`, include a nonempty host and path, and be at most 255 characters. Credentials, query strings, fragments, whitespace and embedded NUL characters are rejected. HTTPS is required when _Allow OTA update only over HTTPS_ is enabled; otherwise the existing HTTP-only URL scheme selection is preserved. The configured server certificate and certificate-name verification are unchanged. Sending `{"url":""}` clears the saved URL and disables updates until another URL is configured. The saved URL is restored after reboot and republished on MQTT reconnect.

Retained or fragmented OTA commands are rejected. Do not retain URL commands or button presses: only URL state and status messages are retained.

| Status | Meaning |
| --- | --- |
| `Ready` / `URL not configured` | URL saved or cleared, or an update was requested without a configured URL. |
| `Update in progress` | The dedicated OTA task is downloading and installing firmware. |
| `Restart requested` | Installation succeeded and a reboot was requested. |
| `Validation pending` | The new image is waiting for the existing connectivity watchdog to validate it. |
| `Firmware validated` | ESP-IDF accepted the new image and cancelled rollback. |
| `Rollback requested` | The existing watchdog requested rollback because MQTT did not reconnect in time. |
| `Failed` | An operation failed; inspect `detail` and local diagnostics. |
| `Restart after crash` | The device restarted after a panic or watchdog reset, not necessarily caused by OTA. |

Statuses are best-effort events, not a persistent transaction history. A network outage can prevent delivery, and the last retained status can outlive an update. Confirm success by checking the running firmware version and `Firmware validated`. A test of this workflow requires a maintenance window, a trusted update server and an available USB recovery path; never use alarm commands to test OTA.