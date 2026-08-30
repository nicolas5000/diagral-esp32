#pragma once

namespace Diagral
{

  // ============================================================================
  // Data Link Layer Constants
  // ============================================================================

  // Frame Sizes
  constexpr uint8_t FRAME_MIN_SIZE = 4;        // bytes = LENGTH (1) + COUNTER (1) + CMD (1) + CRC (1)
  constexpr uint8_t FRAME_DATA_MAX_SIZE = 124; // bytes after CMD and before CRC
  constexpr uint8_t FRAME_MAX_SIZE = 128;      // bytes, total including LENGTH and CRC
  constexpr uint8_t FRAME_ACK_SIZE = 5;        // bytes = LENGTH (1) + COUNTER (1) + CMD (1) + DATA(1) + CRC (1)

  // ============================================================================
  // Command IDs
  // ============================================================================

  // Commands (I = xx)
  constexpr uint8_t CMD_BINARY_MANAGEMENT = 0x04;  // Used to manage binary records (audio?)
  constexpr uint8_t CMD_GENERIC_MANAGEMENT = 0x07; // Generic management command, see J sub commands
  constexpr uint8_t CMD_ACK = 0x40;                // ACK every command

  // Sub commands (J = xx) for binary management (I = 0x04)
  constexpr uint8_t SUBCMD_BINARY_DESCRIPTION_COMMAND = 0x01; // Command to describe binary record before sending fragments
  constexpr uint8_t SUBCMD_BINARY_MAX_FRAG_RESPONSE = 0x02;   // In response to SUBCMD_BINARY_DESCRIPTION_COMMAND, reply fragment max supported size
  constexpr uint8_t SUBCMD_BINARY_FRAGMENT = 0x03;            // Command to send an binary fragment
  constexpr uint8_t SUBCMD_BINARY_END = 0x04;                 // Command to finish binary record transmission (after all fragments are transmitted)
  constexpr uint8_t SUBCMD_BINARY_READ = 0x08;                // Command to ask for binary record
  constexpr uint8_t SUBCMD_BINARY_FRAGMENT_ACK = 0x09;        // Response to acknowledge a received fragment

  // Sub commands (J = xx) for generic management (I = 0x07)
  constexpr uint8_t SUBCMD_GEN_IDLE_GSM = 0x43;               // Command from GSM module when going to idle state
  constexpr uint8_t SUBCMD_GEN_IDLE_SYSTEM = 0x44;            // Command from system when going to idle state
  constexpr uint8_t SUBCMD_GEN_SET_GET_STATE = 0x60;          // Command sent by GSM module to change system state
  constexpr uint8_t SUBCMD_GEN_STATE_NOTIFICATION = 0x62;     // Command from system when changing state
  constexpr uint8_t SUBCMD_GEN_SET_OUTPUT = 0x63;             // Command sent by GSM module to change light or relay state
  constexpr uint8_t SUBCMD_GEN_ALERT_NOTIFICATION = 0x65;     // Command from system on 'alert' event
  constexpr uint8_t SUBCMD_GEN_DETECTION_NOTIFICATION = 0x68; // Command from system on 'detection' event
  constexpr uint8_t SUBCMD_GEN_GSM_NOSIM = 0x69;              // Sent by GSM (seen when no SIM card)
  constexpr uint8_t SUBCMD_GEN_TAMPERING_NOTIFICATION = 0x6B; // Command from system on 'tampering' event
  constexpr uint8_t SUBCMD_GEN_GSM_STATE = 0x6C;              // Sent by GSM, contains system mode
  constexpr uint8_t SUBCMD_GEN_GSM_NOTIFICATION = 0x70;       // Sent by GSM when after SMS or voice call
  constexpr uint8_t SUBCMD_GEN_PIN_MANAGEMENT = 0x71;         // PIN code management (see K sub commands)
  constexpr uint8_t SUBCMD_GEN_POWER_ON_STATE = 0x95;         // Command from system on power on
  constexpr uint8_t SUBCMD_GEN_POWER_ON_RESPONSE = 0x96;      // Sent by GSM as a response to SUBCMD_GEN_POWER_ON_STATE
  constexpr uint8_t SUBCMD_GEN_OTHERS = 0xB0;                 // Others (see K sub commands)

  // Sub commands (K = xx) for PIN management (J = 0x71)
  constexpr uint8_t SUBCMD_PIN_CHANGE_NOTIFICATION = 0x01; // Command from system on PIN change
  constexpr uint8_t SUBCMD_PIN_GET_LENGTH = 0x09;          // Sent by GSM, get PIN length
  constexpr uint8_t SUBCMD_PIN_GET_LENGTH_RESPONSE = 0x0A; // Response from system, PIN length
  constexpr uint8_t SUBCMD_PIN_CHECK = 0x0B;               // Sent by GSM, check PIN code
  constexpr uint8_t SUBCMD_PIN_CHECK_RESPONSE = 0x0C;      // Response from system, PIN code check result

  // Sub commands (K = xx) for "others" management (J = 0xBO)
  constexpr uint8_t SUBCMD_OTHERS_COMMUNICATION_STATE = 0x05;          // Sent by GSM, communication state
  constexpr uint8_t SUBCMD_OTHERS_GSM_06 = 0x06;                       // Sent by GSM, unknown
  constexpr uint8_t SUBCMD_OTHERS_AUDIO_CONTROL = 0x0C;                // Sent by GSM to control system speaker and microphone
  constexpr uint8_t SUBCMD_OTHERS_AUDIO_CONTROL_RESPONSE = 0x0D;       // Response from system to SUBCMD_OTHERS_AUDIO_CONTROL
  constexpr uint8_t SUBCMD_OTHERS_SIGNAL_STATE = 0x0F;                 // Sent by GSM, GSM signal
  constexpr uint8_t SUBCMD_OTHERS_LANGUAGE = 0x10;                     // Command from system, language
  constexpr uint8_t SUBCMD_OTHERS_GSM_11 = 0x11;                       // Sent by GSM, unknown
  constexpr uint8_t SUBCMD_OTHERS_SETTING_CHECK_EXIST = 0x20;          // Command from system, check if a setting exists
  constexpr uint8_t SUBCMD_OTHERS_SETTING_GET = 0x21;                  // Command from system, read setting
  constexpr uint8_t SUBCMD_OTHERS_SETTING_GET_RESPONSE = 0x22;         // Sent by GSM in response to SUBCMD_OTHERS_SETTING_GET
  constexpr uint8_t SUBCMD_OTHERS_SETTING_CHECK_VALUE = 0x23;          // Command from system, check if a value is valid for a given setting
  constexpr uint8_t SUBCMD_OTHERS_SETTING_SET = 0x24;                  // Command from system, write setting
  constexpr uint8_t SUBCMD_OTHERS_SETTING_DEL = 0x25;                  // Command from system, delete setting
  constexpr uint8_t SUBCMD_OTHERS_MISC = 0x27;                         // Command from system, perform some test or configuration
  constexpr uint8_t SUBCMD_OTHERS_RESPONSE = 0x28;                     // Sent by system or GSM in response to a SUBCMD_OTHERS_xxx command or when counter is reset
  constexpr uint8_t SUBCMD_OTHERS_GSM_30 = 0x30;                       // Sent by GSM, unknown
  constexpr uint8_t SUBCMD_OTHERS_SYSTEM_31 = 0x31;                    // Command from system, unknown
  constexpr uint8_t SUBCMD_OTHERS_SYSTEM_40 = 0x40;                    // Command from system, unknown
  constexpr uint8_t SUBCMD_OTHERS_SYSTEM_40_RESPONSE = 0x41;           // Sent by GSM in response to SUBCMD_OTHERS_SYSTEM_40, unknown
  constexpr uint8_t SUBCMD_OTHERS_POWER_NOTIFICATION = 0x42;           // Command from system, power state notification
  constexpr uint8_t SUBCMD_OTHERS_GSM_45 = 0x45;                       // Sent by GSM, unknown
  constexpr uint8_t SUBCMD_OTHERS_STATE_REJECTED = 0x49;               // Command from system, when rejected state change
  constexpr uint8_t SUBCMD_OTHERS_INCOMING_COMMUNICATION_STATE = 0x50; // Sent by GSM, incoming communication state

  // ============================================================================
  // Constants used in commands
  // ============================================================================
  constexpr uint8_t DIAGRAL_DATA_ACK_NO_ERROR = 0x01;                 // ACK with no error
  constexpr uint8_t DIAGRAL_DATA_STATE_SET_DISARM_ALL = 0x10;         // Disarm all zones
  constexpr uint8_t DIAGRAL_DATA_STATE_SET_ARM_ALL = 0x11;            // Arm all zones
  constexpr uint8_t DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL = 0x12; // Arm or Disarm specified zones
  constexpr uint8_t DIAGRAL_DATA_STATE_SET_ARM_HOME = 0x13;           // Arm "home"
  constexpr uint8_t DIAGRAL_DATA_STATE_GET = 0x30;                    // Get Arm / Disarm state
  constexpr uint8_t DIAGRAL_DATA_ZONES_ALL = 0xFF;
  constexpr uint8_t DIAGRAL_DATA_ZONE1 = 0x01;
  constexpr uint8_t DIAGRAL_DATA_ZONE2 = 0x02;
  constexpr uint8_t DIAGRAL_DATA_ZONE3 = 0x04;
  constexpr uint8_t DIAGRAL_DATA_ZONE4 = 0x08;
  constexpr uint8_t DIAGRAL_DATA_STATE_RESPONSE_TO_REQUEST_BIT = 0x01;
  constexpr uint8_t DIAGRAL_DATA_STATE_ARM_HOME_BIT = 0x02;
  constexpr uint8_t DIAGRAL_DATA_STATE_ARM_PARTIAL_BIT = 0x10;
  constexpr uint8_t DIAGRAL_DATA_STATE_ARM_ALL_BIT = 0x40;
  constexpr uint8_t DIAGRAL_DATA_STATE_ARMING_BIT = 0x80;
  constexpr uint8_t DIAGRAL_DATA_POWER_LOST = 0x02;

} // namespace Diagral
