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

Here is an example of discovery message sent to this topic:
```
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
	"availability_topic":	"diagral-esp32/status",
	"cmps":	{
		"reboot":	{
			"p":	"button",
			"unique_id":	"diagral-esp32_button_reboot",
			"name":	"Reboot",
			"command_topic":	"diagral-esp32/button_reboot/set"
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
		"mode":	{
			"p":	"sensor",
			"unique_id":	"diagral-esp32_mode",
			"name":	"Mode",
			"entity_category":	"diagnostic",
			"state_topic":	"diagral-esp32/info",
			"value_template":	"{{ value_json.mode }}"
		},
		"power_supply":	{
			"p":	"sensor",
			"unique_id":	"diagral-esp32_power_supply",
			"name":	"Power supply",
			"entity_category":	"diagnostic",
			"state_topic":	"diagral-esp32/info",
			"value_template":	"{{ value_json.power_supply }}"
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
			"event_types":	["alert", "fire", "silent"],
			"state_topic":	"diagral-esp32/last_alert"
		},
		"last_tamper":	{
			"p":	"event",
			"unique_id":	"diagral-esp32_last_tamper",
			"name":	"Last tamper",
			"event_types":	["tamper", "clear"],
			"state_topic":	"diagral-esp32/last_tamper"
		}
	}
}

```

## Topics and payloads

All topics are prefixed by _MQTT_TOPIC_PREFIX_, that is by default _diagral-uart_  
All topics and payloads are described in discovery messages and compatible with Home Assistant, so they will not be described here. See [Home Assistant MQTT documentation](https://www.home-assistant.io/integrations/mqtt/) for more details.