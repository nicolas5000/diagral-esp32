# Project configuration
The configuration permits to define severals parameters to customize the behavior of the ESP32.
The configuration can be done using several ways:
- Before building the firmware: the default configuration stored in firmware binary
- After building the firmware, while ESP32 is running: configuration is store in NVS partition in Flash and always overrides default configuration.
> [!TIP]
> If you change the default configuration and see no change after loading the firmware, it's probably because you have custom value stored in NVS partition that overrides the default value. You can decide to change directly the value in NVS or delete parameters in NVS to use default values. You can also use ESP-IDF tools to erase the NVS partition in Flash (address 0x370000 and size 0x4000, see [partition.csv](../partitions.csv))

## Ways to configure the project

### Before building the firmware
As previously said, you can choose default values before building the firmware:
- In VSCode: use the ESP-IDF addon by clicking on "SDK Configuration Editor (menuconfig)". Then go to "Diagral UART Project Configuration" section.
- Using a terminal: execute command 'idf.py menuconfig' (see ESP-IDF documentation)
> [!NOTE]
> Some parameters can be defined only before building the firmware, especially parameters related to hardware configuration.
Most of the parameters have a description to help you to choose the value.

#### Main parameters
This section permits to configure:
- Default value for password used to protect command line access
- OTA update using only HTTPS (secure update from known server)
- Wifi or Ethernet connectivity
- DHCP or static IPv4
- ESP32 hostname on your network
- NTP server to use to synchronize time on ESP32

#### Static IP configuration parameters
This section permits to configure:
- IPv4 address for the ESP32
- Network mask
- Gateway address
- Main DNS server address
- Backup DNS server address

#### Wifi configuration parameters
This section permits to configure:
- Wifi SSID
- Wifi password / key
- WPA3 SAE mode (if using WPA3)
- Password identifier (if using SAE H2E)
- WiFi authentication mode threshold: set it directly to the best value supported by your Wifi access point!

#### Ethernet configuration parameters
If you choose Ethernet connectivity in the main parameters, this section permits to configure parameters related to W5500 module:
- SPI host to use (depending on your ESP32 chip, you can have HOST2 and HOST3 or only HOST2)
- SPI clock speed (MHz)
- GPIO to use for:
  - SPI CS
  - Interrupt
  - PHY Reset

#### Diagral configuration parameters
This section permits to configure:
- Enable logging in Diagral UART layer (all logs from the Diagral UART layer will be sent to command line interface, and MQTT log topic if MQTT is configured)
- Passive mode only (ESP32 is not allowed to emit)
- PIN code
- GPIO connected to Diagral system and used for:
  - TX (Pin 8 on Diagral board 20 pins connector)
  - RX (Pin 10 on Diagral board 20 pins connector)
  - TX Flag (Pin 9 on Diagral board 20 pins connector)

#### MQTT configuration parameters
This section permits to configure:
- MQTT client activation
- MQTT Broker address
- MQTT Broker port
- MQTT Client ID (must be unique in your broker)
- MQTT Client username and password to authenticate when connecting to the broker
- TLS protection and certificate when connecting to the broker
- MQTT prefix to use for all topics (except discovery)
- MQTT prefix to use for discovery topic

#### Hardware configuration parameters
This section permits to configure GPIO for SPI HOST2 and/or HOST3 if available and used by Ethernet module:
- SPI SCK
- SPI MOSI
- SPI MISO

#### External power monitoring parameters
This optional section is disabled by default (`CONFIG_DIAGRAL_EXT_POWER_ENABLED`). It is useful when the alarm system is powered through an external UPS: the alarm system then never sees a mains loss and doesn't report it, but the ESP32 can detect it by measuring the external power supply before the UPS. When enabled, an _External power_ binary sensor is exposed to Home Assistant (see [MQTT](mqtt.md)).

This section permits to configure:
- ADC unit and ADC channel connected to the external power measurement
- Threshold (in mV, measured on the ADC pin) above which external power is considered present. A 100 mV hysteresis is applied around this threshold. The input is checked every second.

Wiring: connect the external power supply (for example 5 V, before the UPS input) to the ADC pin through a voltage divider. The voltage on the ADC pin shall always remain under 3.3 V:
```
External power (5 V) ── R1 100 kΩ ──┬── ADC pin
                                    ├── R2 100 kΩ ── GND
                                    └── capacitor 10 nF to 1 µF ── GND
```
With two 100 kΩ resistors, 5 V gives about 2.5 V on the ADC pin, and the default threshold (1800 mV) reports the external power as present above about 3.8 V and absent below about 3.4 V. Keep the divider impedance moderate (a few hundred kΩ at most): some UPS modules leak a small current from the battery back to their input, which shows the battery voltage on a high-impedance voltmeter, but is absorbed by the divider. Check the voltage on the ADC pin with and without external power before relying on the sensor.

### After building the firmware
Most of the parameters, excluding hardware configuration, can be changes using the command line / console interface available on serial interface (or USB to serial interface). Check these commands:
- config_network for configuration related to IPv4 / DHCP / DNS / NTP
- config_wifi for configuration related to Wifi
- config_mqtt for configuration related to MQTT
- diagral_config for configuration related to Diagral
> [!IMPORTANT]
> By default the command line interface is password protected. It strongly recommanded to change the default password (before building the firmware or after loading the firmware in ESP32 chip).
In addition, a few parameters can be changed using MQTT topics.
