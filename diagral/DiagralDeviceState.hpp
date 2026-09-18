#pragma once

#include <ctime>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include "DiagralConstants.hpp"

namespace Diagral
{
    enum DiagralLanguage
    {
        LANGUAGE_UNKNOWN = -1,
        LANGUAGE_FRENCH = 0,
        LANGUAGE_ITALIAN = 1,
        LANGUAGE_GERMAN = 2,
        LANGUAGE_SPANISH = 3,
        LANGUAGE_DUTCH = 4,
        LANGUAGE_ENGLISH = 5
    };
    enum DiagralMode
    {
        DIAGRAL_MODE_IDLE = DIAGRAL_DATA_STATE_MODE_IDLE,
        DIAGRAL_MODE_TEST = DIAGRAL_DATA_STATE_MODE_TEST,
        DIAGRAL_MODE_SETUP = DIAGRAL_DATA_STATE_MODE_SETUP
    };
    enum DiagralState
    {
        DIAGRAL_STATE_DISARMED,
        DIAGRAL_STATE_ARMING,
        DIAGRAL_STATE_ARMED,
        DIAGRAL_STATE_ARMED_HOME,
        DIAGRAL_STATE_TRIGGERED // intrusion detected
    };
    enum DiagralAlertType
    {
        DIAGRAL_ALERT_FIRE = 0x40,
        DIAGRAL_ALERT = 0x50,
        DIAGRAL_ALERT_SILENT = 0x51
    };
    enum DiagralErrorType
    {
        DIAGRAL_ERROR_UNKNOWN = -1,
        DIAGRAL_ERROR_TAMPER,
        DIAGRAL_ERROR_TAMPER_CLEAR,
        DIAGRAL_ERROR_MAIN_POWER_LOST,
        DIAGRAL_ERROR_MAIN_POWER_RESTORED,
        DIAGRAL_ERROR_BATTERY_LOW,
        DIAGRAL_ERROR_BATTERY_RESTORED,
        DIAGRAL_ERROR_RADIO_LOST,
        DIAGRAL_ERROR_RADIO_RESTORED
    };
    enum DiagralErrorHardwareType
    {
        DIGRAL_ERROR_MATERIAL_SYSTEM,
        DIGRAL_ERROR_MATERIAL_SENSOR,
        DIGRAL_ERROR_MATERIAL_COMMAND,
        DIGRAL_ERROR_MATERIAL_SIREN,
    };
    enum DiagralDetectionEventType
    {
        DIAGRAL_DETECTION_EVENT_DISSUASION = 0x07,
        DIAGRAL_DETECTION_EVENT_PRE_ALARM = 0x08,
        DIAGRAL_DETECTION_EVENT_CAUTION = 0x09,
        DIAGRAL_DETECTION_EVENT_INTRUSION = 0x0A,
        DIAGRAL_DETECTION_EVENT_TIMER_START = 0x0B,
        DIAGRAL_DETECTION_EVENT_TIMER_END = 0x0C,
        DIAGRAL_DETECTION_EVENT_PRE_ALARM_CONFIRMED = 0x18,
        DIAGRAL_DETECTION_EVENT_INTRUSION_CONFIRMED = 0x1A
    };
    enum DiagralDetectionSensorType
    {
        DIAGRAL_SENSOR_TIMEOUT = 0x00,
        DIAGRAL_SENSOR_MOVEMENT = 0x01,
        DIAGRAL_SENSOR_OPENING = 0X02
    };
    struct DiagralAlert
    {
        DiagralAlertType type;
        uint8_t commandNumber; // The command that triggered the alert
        time_t timestamp;
    };
    struct DiagralError
    {
        uint8_t hardwareNumber;
        DiagralErrorHardwareType hardwareType;
        DiagralErrorType errorType;
        time_t timestamp;
    };
    struct DiagralDetectionEvent
    {
        DiagralDetectionEventType eventType;
        DiagralDetectionSensorType sensorType;
        uint8_t sensorNumber;
        time_t timestamp;
    };
    struct DiagralDeviceState
    {
        bool error;               // currently in error state
        uint8_t battery;          // current battery status, 0-100%
        DiagralLanguage language; // Current language
        DiagralMode mode;         // Current mode
        DiagralState zone1;
        DiagralState zone2;
        DiagralState zone3;
        DiagralState zone4;
        int64_t lastStateTimestamp; // Timestamp of the last received state, in us (use esp_timer_get_time() to fill and compare to local date&time!)
        DiagralAlert lastAlert;
        DiagralError lastError;
        DiagralDetectionEvent lastDetection;
    };

    /// @brief Convert DiagralMode to string representation
    /// @param mode DiagralMode object to convert
    /// @return string representation
    std::string DiagralModeToString(DiagralMode mode);

    /// @brief Convert DiagralState to string representation
    /// @param state DiagralState object to convert
    /// @return string representation
    std::string DiagralStateToString(DiagralState state);

    /// @brief Convert DiagralAlertType to string representation
    /// @param alert DiagralAlertType object to convert
    /// @return string representation
    std::string DiagralAlertTypeToString(DiagralAlertType alert);

    /// @brief Convert DiagralErrorType to string representation
    /// @param error DiagralErrorType object to convert
    /// @return string representation
    std::string DiagralErrorTypeToString(DiagralErrorType error);

    /// @brief Convert DiagralErrorHardwareType to string representation
    /// @param harware DiagralErrorHardwareType object to convert
    /// @return string representation
    std::string DiagralErrorHardwareTypeToString(DiagralErrorHardwareType harware);

    /// @brief Convert DiagralDetectionEventType to string representation
    /// @param event DiagralDetectionEventType object to convert
    /// @return string representation
    std::string DiagralDetectionEventTypeToString(DiagralDetectionEventType event);

    /// @brief Convert DiagralDetectionSensorType to string representation
    /// @param sensor DiagralDetectionSensorType object to convert
    /// @return string representation
    std::string DiagralSensorTypeToString(DiagralDetectionSensorType sensor);

} // namespace Diagral