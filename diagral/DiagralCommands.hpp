#pragma once

#include <stdint.h>
#include <stddef.h>
#include "DiagralConstants.hpp"
#include "DiagralDeviceState.hpp"

namespace Diagral
{
    /// @brief Diagral Frame structure
    struct DiagralFrame
    {
        uint8_t data_length;               // Frame Length (1 byte)
        uint8_t data[FRAME_DATA_MAX_SIZE]; // Data
    };

    /// @brief Create a I=07/J=43 Frame
    /// @param frame frame to create
    /// @return true on success
    bool create_J43_frame(DiagralFrame &frame);

    /// @brief Create a I=07/J=95 Frame
    /// @param frame response to create
    /// @return true on success
    bool create_J95_response(DiagralFrame &frame);

    /// @brief Create a I=07/J=96 Frame
    /// @param frame response to create
    /// @return true on success
    bool create_J96_response(DiagralFrame &frame);

    /// @brief Create a I=07/J=6C Frame
    /// @param frame response to create
    /// @param idle true if system is in idle mode
    /// @return true on success
    bool create_gsm_state_response(DiagralFrame &frame, bool idle);

    /// @brief Create a I=07/J=B0/K=06 Frame
    /// @param frame frame to create
    /// @return true on success
    bool create_K06_frame(DiagralFrame &frame);

    /// @brief Create a I=07/J=B0/K=0C Frame
    /// @param frame frame to create
    /// @return true on success
    bool create_K0C_frame(DiagralFrame &frame);

    /// @brief Create a I=07/J=B0/K=11 Frame
    /// @param frame frame to create
    /// @param second_payload fill the frame with second known payload
    /// @return true on success
    bool create_K11_frame(DiagralFrame &frame, bool second_payload);

    /// @brief Create a I=07/J=B0/K=30 Frame
    /// @param frame frame to create
    /// @param byte1 byte1 to send
    /// @param byte2 byte2 to send
    /// @param byte3 byte3 to send
    /// @return true on success
    bool create_K30_frame(DiagralFrame &frame, uint8_t byte1, uint8_t byte2, uint8_t byte3);

    /// @brief Create a I=07/J=B0/K=41 Frame
    /// @param frame response to create
    /// @return true on success
    bool create_K41_response(DiagralFrame &frame);

    /// @brief Create a I=04/J=02 Frame
    /// @param frame response to create
    /// @return true on success
    bool create_binary_max_fragment_response(DiagralFrame &frame);

    /// @brief Create a I=04/J=09 Frame
    /// @param frame response to create
    /// @param fragmentIndex fragment index to acknowledge
    /// @return true on success
    bool create_binary_fragment_ack(DiagralFrame &frame, uint16_t fragmentIndex);

    /// @brief Create a I=04/J=01 Frame in response to a I=04/J=08 Frame
    /// @param frame I=04/J=01 response to create
    /// @param requestFrame I=04/J=08 Frame
    /// @return true on success
    bool create_binary_response_for_read_binary(DiagralFrame &frame, const DiagralFrame &requestFrame);

    /// @brief Create a I=04/J=03 Frame
    /// @param frame Frame to create
    /// @return true on success
    bool create_binary_fragment_request(DiagralFrame &frame);

    /// @brief Create a I=04/J=04 Frame
    /// @param frame Frame to create
    /// @return true on success
    bool create_binary_end_request(DiagralFrame &frame);

    /// @brief Create a I=07/J=60 for getting or setting current state
    /// @param frame Frame to create
    /// @param command Command to execute: DIAGRAL_DATA_STATE_SET_DISARM_ALL, DIAGRAL_DATA_STATE_SET_ARM_ALL,
    /// DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL, DIAGRAL_DATA_STATE_SET_ARM_HOME or DIAGRAL_DATA_STATE_GET
    /// @param disarmZones Zones to disarm (in case of DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL command)
    /// @param armZones Zones to arm (in case of DIAGRAL_DATA_STATE_SET_ARM_DISARM_PARTIAL command)
    /// @return true on success
    bool create_get_set_state_command(DiagralFrame &frame, uint8_t command, uint8_t disarmZones = 0, uint8_t armZones = 0);

    /// @brief Create a I=07/J=71/K=09 command to get PIN length from Diagral System
    /// @param frame Frame to create
    /// @return true on success
    bool create_pin_get_length_command(DiagralFrame &frame);

    /// @brief Create a I=07/J=71/K=0B command to check PIN code in Diagral System
    /// @param frame Frame to create
    /// @param pinCode PIN code to check, eg 0x12 0x34 0x00 to check PIN "1234"
    /// @return true on success
    bool create_pin_check_command(DiagralFrame &frame, std::string pinCode);

    /// @brief Create a xxx Frame
    /// @param frame Output DiagralFrame structure
    /// @return true on success
    bool create_xxx_request(DiagralFrame &frame);

    /// @brief Process xxx response
    /// @param frame Input DiagralFrame structure
    /// @param device The structure to fill from Diagral Frame
    /// @return true on success
    bool process_xxx(const DiagralFrame &frame, DiagralDeviceState &device);
} // namespace Diagral