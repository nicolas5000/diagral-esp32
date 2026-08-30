#include "DiagralCommands.hpp"
#include <string.h>
#include <stdlib.h>

namespace Diagral
{
    bool create_J96_response(DiagralFrame &frame)
    {
        // Create response from what we observed from GSM module
        uint8_t data[15] = {CMD_GENERIC_MANAGEMENT, SUBCMD_GEN_POWER_ON_RESPONSE, 0x81, 0x02, 0x0A, 0x26, 0x6E, 0x24, 0x01, 0x90, 0x31, 0x01, 0x01, 0x00, 0x00};
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_K41_response(DiagralFrame &frame)
    {
        uint8_t data[4] = {CMD_GENERIC_MANAGEMENT, SUBCMD_GEN_OTHERS, SUBCMD_OTHERS_SYSTEM_40_RESPONSE, 0x00};
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_binary_max_fragment_response(DiagralFrame &frame)
    {
        uint8_t data[6] = {CMD_BINARY_MANAGEMENT, 0x20, SUBCMD_BINARY_MAX_FRAG_RESPONSE, 0x00, 0x57, 0x00};
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_binary_fragment_ack(DiagralFrame &frame, uint16_t fragmentIndex)
    {
        uint8_t data[5] = {CMD_BINARY_MANAGEMENT, 0x20, SUBCMD_BINARY_FRAGMENT_ACK, 0x00, 0x00};
        data[3] = (fragmentIndex >> 8) & 0xFF;
        data[4] = fragmentIndex & 0xFF;
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_binary_response_for_read_binary(DiagralFrame &frame, const DiagralFrame &requestFrame)
    {
        if (requestFrame.data_length < 9)
            return false;
        uint8_t data[14];
        memset(data, 0, sizeof(data));
        data[0] = CMD_BINARY_MANAGEMENT;
        data[1] = 0x02;
        data[2] = SUBCMD_BINARY_DESCRIPTION_COMMAND;
        memcpy(data + 3, requestFrame.data + 3, 6);
        // data[9] = 0x00;
        // data[10] = 0x00;
        data[11] = 0x22;
        // data[12] = 0x00;
        data[13] = 0x57;
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_binary_fragment_request(DiagralFrame &frame)
    {
        uint8_t data[41] = {CMD_BINARY_MANAGEMENT, 0x20, SUBCMD_BINARY_FRAGMENT, 0x00, 0x01, 0x00, 0x22, 0x00, 0x1E, 0x00, 0x00, 0x22, 0x00, 0x0C, 0xBA, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x26, 0x6E, 0xFF, 0xFF, 0x24, 0x01, 0x90, 0x31, 0xFF, 0xFF, 0xFF, 0x55, 0x55, 0xFF, 0xFF, 0xFF, 0xFF};
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_binary_end_request(DiagralFrame &frame)
    {
        uint8_t data[3] = {CMD_BINARY_MANAGEMENT, 0x20, SUBCMD_BINARY_END};
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_get_set_state_command(DiagralFrame &frame, uint8_t command, uint8_t disarmZones, uint8_t armZones)
    {
        uint8_t data[11] = {CMD_GENERIC_MANAGEMENT, SUBCMD_GEN_SET_GET_STATE, 0x00, command, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        switch (command)
        {
        case DIAGRAL_DATA_STATE_SET_DISARM_ALL:
            data[4] = DIAGRAL_DATA_ZONES_ALL;
            data[5] = DIAGRAL_DATA_ZONES_ALL;
            break;
        case DIAGRAL_DATA_STATE_SET_ARM_ALL:
            data[6] = DIAGRAL_DATA_ZONES_ALL;
            data[7] = DIAGRAL_DATA_ZONES_ALL;
            break;
        case DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL:
            data[5] = disarmZones;
            data[7] = armZones;
        case DIAGRAL_DATA_STATE_SET_ARM_HOME:
        case DIAGRAL_DATA_STATE_GET:
            // Nothing to set except command byte
            break;
        default:
            return false;
        }
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_pin_get_length_command(DiagralFrame &frame)
    {
        uint8_t data[3] = {CMD_GENERIC_MANAGEMENT, SUBCMD_GEN_PIN_MANAGEMENT, SUBCMD_PIN_GET_LENGTH};
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_pin_check_command(DiagralFrame &frame, std::string pinCode)
    {
        uint8_t data[6] = {CMD_GENERIC_MANAGEMENT, SUBCMD_GEN_PIN_MANAGEMENT, SUBCMD_PIN_CHECK, 0x00, 0x00, 0x00};
        // Copy PIN code
        const char *code = pinCode.c_str();
        for (int8_t i = 0; i < pinCode.length(); i++)
        {
            if (code[i] < '0' || code[i] > '9') return false;
            data[3 + i / 2] = data[3 + i / 2] | ((code[i] - '0') << (( i % 2 == 0) ? 4 : 0));
        }
        // Prepare DiagralFrame
        memcpy(frame.data, data, sizeof(data));
        frame.data_length = sizeof(data);
        return true;
    }

    bool create_xxx_request(DiagralFrame &frame)
    {
        // TODO
        return true;
    }

    bool process_xxx(const DiagralFrame &frame, DiagralDeviceState &device)
    {
        // TODO
        return true;
    }

} // namespace Diagral