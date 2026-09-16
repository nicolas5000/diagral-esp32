# Communication protocol between DIAG91AGFK alarm system and DIAG55AAX GSM module
This page describes most of the protocol used between DIAG91AGFK alarm system and DIAG55AAX GSM module... at least useful commands to control and monitor the alarm system from this UART link.

## Hardware

### Pinout
As described in [Diagral DIAG91AGFK 20-pins connector](../README.md#diagral-diag91agfk-20-pins-connector), the 3 pins to control and monitor the alarm system are:
- Pin 8: UART RX for DIAG91AGFK / TX for DIAG55AAX
- Pin 9: "Signal" pin, low by default, set to 2.8V by DIAG91AGFK or DIAG55AAX a few ms before sending any command on corresponding TX (wake up?)
- Pin 10: UART TX for DIAG91AGFK / RX for DIAG55AAX

### UART
The UART characteritics are 115200 bps, 8 bits of data, no parity bit, 1 stop bit.

## Protocol

### Frame format

The format of each frame is:
| Byte # | Description |
|----|---|
| 0 | L = Length of the frame (this byte excluded) |
| 1 | Frame counter, from 0x00 to 0xFF, independent and incremented by each side (one for DIAG91AGFK, one for DIAG55AAX/ESP32) |
| 2 | I = Identifier of the frame |
| ... | ... |
| L | Checksum (XOR of all previous bytes, byte 0 included) |

> [!NOTE]
> In the following of this document, the frame length (byte 0), frame counter (byte 1) and checksum (last byte) will not always be shown for easy reading, however they are always present in the frames.

### Frame description

#### I=0x40 - ACK
All frames sent from one side must be acknowledged by the other side by sending this frame:
| Byte # | Description |
|----|---|
| 0 | L = Length of the frame (this byte excluded) = 0x04 |
| 1 | Frame counter of the frame to acknowledge |
| 2 | I = 0x40 |
| 3 | 0x01 (OK) |
| 4 | Checksum (XOR of all previous bytes, byte 0 included) |

#### I=0x04 - Commands to manage audio or binary blobs
> [!NOTE]
> This project doesn't use these audio or binary blobs, however we need to at least reply correctly to the frames from the alarm system.

| Byte # | Description |
|----|---|
| 0 | L = Length of the frame (this byte excluded) |
| 1 | Frame counter |
| 2 | I = Identifier of the frame |
| 3 | Always 0x20 |
| 4 | J Sub-command (see below) |
| ... | ... |
| L | Checksum (XOR of all previous bytes, byte 0 included) |

##### J=0x01 - Description of an audio or binary blob
This frame is used to describe an audio or binary blob, before transmitting the fragments.
| Byte # | Description |
|----|---|
| 2 | I = 0x04 |
| 3 | Always 0x20 |
| 4 | J = 0x01 |
| 5-11 | TBD |
| 12-13 | Total length of the audio or binary blob in bytes, MSB first |
| 14-15 | Maximum supported length of a segment in bytes, MSB first |

##### J=0x02 - Length of a segment (response to J=0x01)
This frame is a response to [J=0x01](#j0x01---description-of-an-audio-or-binary-blob), it contains the maximum supported length of a segment from the other side.
| Byte # | Description |
|----|---|
| 2 | I = 0x04 |
| 3 | Always 0x20 |
| 4 | J = 0x02 |
| 5-6 | Maximum supported length of a segment in bytes, MSB first |
| 7 | 0x00 - Only sent by the GSM module to accept the [J=0x01](#j0x01---description-of-an-audio-or-binary-blob) command to write the blob |

##### J=0x03 - Transmission of a fragment
This frame permits to transmit a fragment of a blob (part of the welcome message, personalized message for a sensor, ...)
| Byte # | Description |
|----|---|
| 2 | I = 0x04 |
| 3 | Always 0x20 |
| 4 | J = 0x03 |
| 5-6 | Fragment #, MSB first, strating from 1 |
| 7-8 | L = Length of the segment in bytes, MSB first |
| ... | Value (L bytes) |

##### J=0x04 - End of transmission of an audio or binary blob (after last J=0x03)
After sending all the fragments with [J=0x03](#j0x03---transmission-of-a-fragment), the sender emits this frame.
| Byte # | Description |
|----|---|
| 2 | I = 0x04 |
| 3 | Always 0x20 |
| 4 | J = 0x04 |

##### J=0x08 - Request to send an audio or binary blob
This frame is sent by the alarm system to the GSM module to read / extract a blob.
| Byte # | Description |
|----|---|
| 2 | I = 0x04 |
| 3 | Always 0x20 |
| 4 | J = 0x08 |
| 5-10 | TBD, same bytes as bytes 5-10 from [J=0x01](#j0x01---description-of-an-audio-or-binary-blob) |

##### J=0x09 - ACK of a segment (response to J=0x03)
This frame is sent to acknowldge each [fragment received](#j0x03---transmission-of-a-fragment).
| Byte # | Description |
|----|---|
| 2 | I = 0x04 |
| 3 | Always 0x20 |
| 4 | J = 0x09 |
| 5-6 | fragment # to acknowledge |

#### I=0x07 - Main commands
This section contains all the other commands to control and get status and notifications from the alarm system.
| Byte # | Description |
|----|---|
| 0 | L = Length of the frame (this byte excluded) |
| 1 | Frame counter |
| 2 | I = 0x07 |
| 3 | J Sub-command (see below) |
| ... | ... |

##### J=0x60 - Command to change the alarm system state
This frame is sent by the GSM module to get or change the alarm system state. The request contains 11 data bytes, so byte 0 (L) is 0x0D.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x60 |
| 4 | 0x00 |
| 5 | Operation code (see below) |
| 6-12 | Reserved bytes and zone masks, depending on the operation |

The operation codes are:
| Value | Operation | Zone bytes |
|----|----|----|
| 0x10 | Disarm all zones | Bytes 6-7 = 0xFF |
| 0x11 | Arm all zones | Bytes 8-9 = 0xFF |
| 0x12 | Arm or disarm selected zones | Byte 7 = zones to disarm; byte 9 = zones to arm |
| 0x13 | Arm the home zones | No zone mask |
| 0x30 | Get the current state | No zone mask |

Zone masks use one bit per zone: 0x01 = zone 1, 0x02 = zone 2, 0x04 = zone 3, and 0x08 = zone 4. 0xFF can be used to select all zones.

A successful command is normally acknowledged with a miscellaneous response (K=0x28) and a status of 0x01. After that, the alarm system then returns a [J=0x62 status notification](#j0x62---status-notification-from-the-alarm-system), or refuses the change with [K=0x49](#k0x49---state-change-refused).
##### J=0x62 - Status notification from the alarm system
This notification is sent by the alarm system when its state changes. It can also be returned in response to a [J=0x60 state request](#j0x60---command-to-change-the-alarm-system-state). The payload contains 18 data bytes, so byte 0 (L) is 0x12.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x62 |
| 4 | System mode: 0x00 = idle, 0x40 = test, 0x80 = setup |
| 5 | State flags (see below) |
| 6 | Error flag: bit 1 (0x02) indicates an error |
| 7 | Not interpreted by this project |
| 8 | Zone mask used when the system is disarmed |
| 9 | Not interpreted by this project |
| 10 | Zone mask used when the system is armed or arming |
| 11-17 | Not interpreted by this project |

The state flags in byte 5 use the following bits:
| Bit | Value | Meaning |
|----|----|----|
| 0 | 0x01 | Response to a state request |
| 1 | 0x02 | Home mode is active |
| 4 | 0x10 | Partial arm is active |
| 6 | 0x40 | Full arm is active |
| 7 | 0x80 | The system is arming |

The zone masks in bytes 8 and 10 use the same bit assignment as the [J=0x60 command](#j0x60---command-to-change-the-alarm-system-state).
##### J=0x65 - Alert notification from the alarm system
This notification is sent by the alarm system when an alert is triggered. The payload contains 13 data bytes, so byte 0 (L) is 0x0D.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x65 |
| 4 | Not interpreted by this project |
| 5 | Alert type: 0x40 = fire, 0x50 = alert, 0x51 = silent alert |
| 6-8 | Not interpreted by this project |
| 9 | Number of the command that triggered the alert |
| 10+ | Not interpreted by this project |

The ESP32 records the alert type, command number, and reception time. No application-level response is required; the frame is acknowledged at the protocol link layer.
##### J=0x68 - Detection notification from the alarm system
This notification is sent by the alarm system when a sensor event occurs. The payload contains 13 data bytes, so byte 0 (L) is 0x0D.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x68 |
| 4 | Not interpreted by this project |
| 5 | Detection event type (see below) |
| 6 | Zone mask for the affected zones |
| 7-9 | Not interpreted by this project |
| 10 | Sensor type: 0x00 = timeout, 0x01 = movement, 0x02 = opening |
| 11 | Sensor number |
| 12+ | Not interpreted by this project |

The known event types are:
| Value | Event |
|----|----|
| 0x07 | Dissuasion |
| 0x08 | Pre-alarm |
| 0x09 | Caution |
| 0x0A | Intrusion |
| 0x0B | Timer started |
| 0x0C | Timer ended |
| 0x18 | Pre-alarm confirmed |
| 0x1A | Intrusion confirmed |

The zone mask uses bit 0 for zone 1, bit 1 for zone 2, bit 2 for zone 3, and bit 3 for zone 4. The ESP32 marks the corresponding zones as triggered and records the event type, sensor type, sensor number, and reception time.
##### J=0x6B - Error notification from the alarm system
This notification is sent by the alarm system when an error or fault is raised or cleared. The payload contains 12 data bytes, so byte 0 (L) is 0x0C.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x6B |
| 4 | Not interpreted by this project |
| 5 | Error state: 0x00 = cleared/restored, non-zero = active |
| 6 | Error type (see below) |
| 7 | Hardware type (see below) |
| 8 | Not interpreted by this project |
| 9 | Hardware number |
| 10+ | Not interpreted by this project |

The known error types are:
| Value | Meaning when active | Meaning when byte 5 is 0x00 |
|----|----|----|
| 0x40 | Tamper active | Tamper cleared |
| 0x59 | Radio link lost | Radio link restored |
| 0x70 | Battery low | Battery restored |
| 0x71 | GSM battery low | GSM battery restored |

The known hardware types are:
| Value | Hardware |
|----|----|
| 0x14 | Alarm system |
| 0x20 | Command module |
| 0x30 or 0x31 | Sensor |

The ESP32 records the error state, hardware type, hardware number, and reception time. Main power events are reported separately with [K=0x42](#k0x42---alarm-system-power-notification).
##### J=0x71 - PIN code management
This command family is used to read the configured PIN length and check a PIN code before sending a state-changing command.
###### K=0x09 - Request for PIN code length
This frame is sent by the GSM module to request the length of the PIN code.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x71 |
| 4 | K = 0x09 |
###### K=0x0A - PIN code length response
This frame is returned by the alarm system in response to K=0x09.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x71 |
| 4 | K = 0x0A |
| 5 | PIN code length in digits |
###### K=0x0B - Request for PIN code check
This frame is sent by the GSM module to check a PIN code. The PIN is encoded as BCD, with two decimal digits per byte and the first digit in the high nibble. The protocol supports up to six digits and pads an unused final byte with 0x00.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x71 |
| 4 | K = 0x0B |
| 5 | PIN digits 1 and 2 |
| 6 | PIN digits 3 and 4 |
| 7 | PIN digits 5 and 6 |

For example, PIN `1234` is encoded as `0x12 0x34 0x00`. The input must contain decimal digits only.
###### K=0x0C - PIN code check status
This frame is returned by the alarm system in response to K=0x0B.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x71 |
| 4 | K = 0x0C |
| 5 | 0x00 = PIN accepted; any other value = PIN rejected |
##### J=0x95 - Unknown from alarm system
This frame is sent by the alarm system during startup or power-on. Its payload is not interpreted by this project, but the frame requires a response from the GSM module.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x95 |
| 4+ | Startup or power-on payload, not decoded |
##### J=0x96 - Unknown from GSM module
This is the response sent by the GSM module to a [J=0x95 frame](#j0x95---unknown-from-alarm-system). The following payload is an observed response that is reused in this project; the meaning of its individual bytes has not been determined.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0x96 |
| 4 | 0x81 |
| 5 | 0x02 |
| 6 | 0x0A |
| 7 | 0x26 |
| 8 | 0x6E |
| 9 | 0x24 |
| 10 | 0x01 |
| 11 | 0x90 |
| 12 | 0x31 |
| 13 | 0x01 |
| 14 | 0x01 |
| 15 | 0x00 |
| 16 | 0x00 |
##### J=0xB0 - Misc commands
This section contains miscellaneous commands and notifications.
###### K=0x10 - Language
This notification is sent by the alarm system to report its language.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0xB0 |
| 4 | K = 0x10 |
| 5 | Not interpreted by this project |
| 6 | Language code: 0x00 = French, 0x01 = Italian, 0x02 = German, 0x03 = Spanish, 0x04 = Dutch, 0x05 = English |
###### K=0x40 - Unknown from alarm system
This frame is sent by the alarm system during initialization. Its payload is not interpreted by this project, but the frame requires a response from the GSM module.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0xB0 |
| 4 | K = 0x40 |
###### K=0x41 - Unknown from GSM module
This is the response sent by the GSM module to [K=0x40](#k0x40---unknown-from-alarm-system).
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0xB0 |
| 4 | K = 0x41 |
| 5 | 0x00 |
###### K=0x42 - Alarm system power notification
This notification is sent by the alarm system to report a change in its power or battery state.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0xB0 |
| 4 | K = 0x42 |
| 5 | Power state: 0x00 or 0x02 = main power lost, 0x01 = main power restored, 0x03 = battery low |
| 6+ | Not interpreted by this project |

The controller records the event as a system error with hardware number 0. Other power-state values are treated as unknown.
###### K=0x49 - State change refused
This frame is sent in response to [J=0x60](#j0x60---command-to-change-the-alarm-system-state) to refuse to change the state, mainly due to errors (tamper...) or open doors/windows.
| Byte # | Description |
|----|---|
| 2 | I = 0x07 |
| 3 | J = 0xB0 |
| 4 | K = 0x49 |
| 5 | 0x03 = refused to change alarm system state |