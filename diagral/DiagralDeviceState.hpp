#pragma once

#include <ctime>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include "DiagralConstants.hpp"

namespace Diagral
{
    enum DiagralPowerSupply
    {
        POWER_UNKNOWN = -1,
        POWER_NO_MAINS = 0,
        POWER_MAINS = 1
    };
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
        DIAGRAL_MODE_IDLE = 0x00, // Idle (not test, not setup mode)
        DIAGRAL_MODE_TEST = 0x40, // Test mode
        DIAGRAL_MODE_SETUP = 0x80 // Setup / installation mode
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
    enum DiagralSensorType
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
    struct DiagralTamper
    {
        uint8_t sensorNumber; // 0 for Diagral system, then 1 to ... for sensors
        bool isActive;        // 1 if tamper is active, 0 if terminated
        time_t timestamp;
    };
    struct DiagralDetectionEvent
    {
        DiagralDetectionEventType eventType;
        DiagralSensorType sensorType;
        uint8_t sensorNumber;
        time_t timestamp;
    };
    struct DiagralDeviceState
    {
        DiagralPowerSupply power; // Current power supply status
        uint8_t battery;          // current battery status, 0-100%
        DiagralLanguage language; // Current language
        DiagralMode mode;         // Current mode
        DiagralState zone1;
        DiagralState zone2;
        DiagralState zone3;
        DiagralState zone4;
        int64_t lastStateTimestamp; // Timestamp of the last received state, in us (use esp_timer_get_time() to fill and compare to local date&time!)
        DiagralAlert lastAlert;
        DiagralTamper lastTamper;
        DiagralDetectionEvent lastDetection;
    };

    /// @brief Convert DiagralPowerSupply to string representation
    /// @param power DiagralPowerSupply object to convert
    /// @return string representation
    std::string DiagralPowerSupplyToString(DiagralPowerSupply power);

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

    /// @brief Convert DiagralDetectionEventType to string representation
    /// @param event DiagralDetectionEventType object to convert
    /// @return string representation
    std::string DiagralDetectionEventTypeToString(DiagralDetectionEventType event);

    /// @brief Convert DiagralSensorType to string representation
    /// @param sensor DiagralSensorType object to convert
    /// @return string representation
    std::string DiagralSensorTypeToString(DiagralSensorType sensor);

} // namespace Diagral