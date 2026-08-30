# Command line usage

> [!NOTE]
> By default command line is protected by a password, see [configuration](doc/configuration.md#main-parameters) to change this password.

## Help

After login to command line, you can access different commands explained below.
The command `help` permits to display all commands available in running firmware.
```
help  [<string>] [-v <0|1>]
  Print the summary of all registered commands if no arguments are given,
  otherwise print summary of given command.
      <string>  Name of command
  -v, --verbose=<0|1>  If specified, list console commands with given verbose level
```

## Misc commands

### Reboot
```
reboot 
  Reboot ESP32
```
### Logout
```
logout 
  Logout from ESP32 console
```

## Configuration commands

### Configure command line password
```
config_password  [-d] [--pass=<password>]
  Configure console password (change is applied immediately)
  -d, --delete  Delete current configuration in storage (no other argument required)
  --pass=<password>  Password (up to 32 characters)
```

### Configure Wifi
```
config_wifi  [-rd] [--ssid=<SSID>] [--pwd=<password>] [--saemode=<SAE mode>] [--saepwid=<SAE pass>] [--auth=<threshold>]
  Configure Wifi (changes are applied after reboot)
    -r, --read  Read current configuration from storage (no other argument required)
  -d, --delete  Delete current configuration in storage (no other argument required)
  --ssid=<SSID>  Wifi SSID
  --pwd=<password>  Wifi password
  --saemode=<SAE mode>  Integer value: 1 = HUNT AND PECK, 2 = H2E, 3 = BOTH
  --saepwid=<SAE pass>  SAE password identifier
  --auth=<threshold>  Authentication threshold: OPEN, WEP, WPA-PSK, WPA/WPA2-PSK, WPA2-PSK, WAPI-PSK, WPA2/WPA3-PSK, WPA3-PSK
```

### Configure network
```
config_network  [-rd] [--hostname=<hostname>] [--dhcp=<dhcp>] [--ip=<address>] [--mask=<netmask>] [--gateway=<gateway>] [--dns1=<DNS1 address>] [--dns2=<DNS2 address>] [--ntp=<NTP address>]
  Configure Network (changes are applied after reboot)
    -r, --read  Read current configuration from storage (no other argument required)
  -d, --delete  Delete current configuration in storage (no other argument required)
  --hostname=<hostname>  ESP32 hostname
  --dhcp=<dhcp>  1 to enabled DHCP, 0 to disable (static IP)
  --ip=<address>  ESP32 IPv4 address for static configuration
  --mask=<netmask>  Network mask for static configuration
  --gateway=<gateway>  Gateway IPv4 address for static configuration
  --dns1=<DNS1 address>  Main DNS server address for static configuration
  --dns2=<DNS2 address>  Backup DNS server address for static configuration
  --ntp=<NTP address>  NTP server address (eg pool.ntp.org)
```

### Configure MQTT client
```
config_mqtt  [-rd] [--state=<state>] [--addr=<address>] [--port=<port>] [--id=<client_id>] [--user=<username>] [--pass=<password>] [--tls=<tls_state>] [--cert=<certificate>] [--topic=<topic_prefix>] [--discovery=<discovery_prefix>]
  Configure MQTT (changes are applied after reboot)
    -r, --read  Read current configuration from storage (no other argument required)
  -d, --delete  Delete current configuration in storage (no other argument required)
  --state=<state>  1 to enable MQTT client, 0 to disable
  --addr=<address>  Broker address to connect to
  --port=<port>  Broker port to connect to
  --id=<client_id>  Client unique ID when connecting to MQTT broker
  --user=<username>  Client username when connecting to MQTT broker
  --pass=<password>  Client password when connecting to MQTT broker
  --tls=<tls_state>  1 to enable TLS connection to MQTT broker, 0 to disable
  --cert=<certificate>  MQTT broker certificate (content of .pem file without --- BEGIN CERTIFICATE --- and ---END CERTIFICATE ---)
  --topic=<topic_prefix>  Prefix added before all MQTT topics except discovery
  --discovery=<discovery_prefix>  Prefix added before discovery topic. Discovery topic will be <discovery_prefix>/config
```

### Configure Syslog client
```
config_syslog  [-rd] [--enable=<0|1>] [--server=<host>] [--port=<port>] [--facility=<0-23>] [--level=<0-7>]
  Configure remote UDP syslog (RFC 3164)
    -r, --read  Read current syslog configuration
  -d, --delete  Delete syslog configuration (restore defaults)
  --enable=<0|1>  1 to enable syslog, 0 to disable
  --server=<host>  Syslog server hostname or IP
  --port=<port>  UDP port (default 514)
  --facility=<0-23>  RFC 3164 facility number
  --level=<0-7>  Minimum severity: 3=error 4=warn 6=info 7=debug
```

## Diagral related commands

### Diagral configuration
```
diagral_config  [-rd] [--logging=<state>] [--passive=<state>] [--checkpin=<state>]
  Configure Diagral controller layer
    -r, --read  Read current configuration from storage (no other argument required)
  -d, --delete  Delete current configuration in storage (no other argument required)
  --logging=<state>  1 to enable logging in Diagral controller layer, 0 to disable
  --passive=<state>  1 to enable passive state in Diagral controller layer, 0 to disable
  --checkpin=<state>  1 to enable PIN code check from MQTT
```

### "Get state" action
```
diagral_get_state 
  Get current state from Diagral system
```

### "Check PIN" action
```
diagral_checkpin  <pin code>
  Check a PIN code in Diagral system
    <pin code>  PIN code, 4 to 6 digits
```

### "Arm all zones" action
```
diagral_arm_all 
  Arm all zones in Diagral system
```

### "Arm specified zones" action
```
diagral_arm_zones  <zones>
  Arm specified zones in Diagral system
       <zones>  Specify the zone(s) to arm as a bit field (1 for zone 1, 2 for zone 2, 4 for zone 3, 8 for zone 4, 3 for zones 1 and 2, ...)
```

### "Arm Home" action
```
diagral_arm_home 
  Arm 'Home presence' in Diagral system
```

### "Disarm all zones" action
```
diagral_disarm_all 
  Disarm all zones in Diagral system
```

### "Disarm specified zones" action
```
diagral_disarm_zones  <zones>
  Disarm specified zones in Diagral system
       <zones>  Specify the zone(s) to disarm as a bit field (1 for zone 1, 2 for zone 2, 4 for zone 3, 8 for zone 4, 3 for zones 1 and 2, ...)
```

### "Send raw command" action
```
diagral_sendraw  <raw frame>
  Send an Digral frame from given string representation, waits for ACK.
   <raw frame>  String representation of the DiagralFrame, from Identifier byte to last byte of data (without Length, counter and CRC)
```
