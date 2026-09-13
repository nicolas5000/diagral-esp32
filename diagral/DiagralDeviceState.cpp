#include "DiagralDeviceState.hpp"

namespace Diagral
{
    std::string DiagralModeToString(DiagralMode mode)
    {
        switch (mode)
        {
        case DiagralMode::DIAGRAL_MODE_IDLE:
            return "Idle";
        case DiagralMode::DIAGRAL_MODE_SETUP:
            return "Setup";
        case DiagralMode::DIAGRAL_MODE_TEST:
            return "Test";
        default:
            return "unknown";
        }
    }
    std::string DiagralStateToString(DiagralState state)
    {
        switch (state)
        {
        case DiagralState::DIAGRAL_STATE_ARMED:
            return "armed_away";
        case DiagralState::DIAGRAL_STATE_ARMED_HOME:
            return "armed_home";
        case DiagralState::DIAGRAL_STATE_ARMING:
            return "arming";
        case DiagralState::DIAGRAL_STATE_DISARMED:
            return "disarmed";
        case DiagralState::DIAGRAL_STATE_TRIGGERED:
            return "triggered";
        default:
            return "unknown";
        }
    }
    std::string DiagralAlertTypeToString(DiagralAlertType alert)
    {
        switch (alert)
        {
        case DiagralAlertType::DIAGRAL_ALERT:
            return "alert";
        case DiagralAlertType::DIAGRAL_ALERT_FIRE:
            return "fire";
        case DiagralAlertType::DIAGRAL_ALERT_SILENT:
            return "silent";
        default:
            return "unknown";
        }
    }
    std::string DiagralErrorTypeToString(DiagralErrorType error)
    {
        switch (error)
        {
        case DiagralErrorType::DIAGRAL_ERROR_TAMPER:
            return "tamper";
        case DiagralErrorType::DIAGRAL_ERROR_TAMPER_CLEAR:
            return "tamper_clear";
        case DiagralErrorType::DIAGRAL_ERROR_MAIN_POWER_LOST:
            return "main_power_lost";
        case DiagralErrorType::DIAGRAL_ERROR_MAIN_POWER_RESTORED:
            return "main_power_restored";
        case DiagralErrorType::DIAGRAL_ERROR_BATTERY_LOW:
            return "battery_low";
        case DiagralErrorType::DIAGRAL_ERROR_BATTERY_RESTORED:
            return "battery_restored";
        case DiagralErrorType::DIAGRAL_ERROR_RADIO_LOST:
            return "radio_lost";
        case DiagralErrorType::DIAGRAL_ERROR_RADIO_RESTORED:
            return "radio_restored";
        default:
            return "unknown";
        }
    }
    std::string DiagralErrorHardwareTypeToString(DiagralErrorHardwareType harware)
    {
        switch (harware)
        {
        case DiagralErrorHardwareType::DIGRAL_ERROR_MATERIAL_SYSTEM:
            return "system";
        case DiagralErrorHardwareType::DIGRAL_ERROR_MATERIAL_SENSOR:
            return "sensor";
        case DiagralErrorHardwareType::DIGRAL_ERROR_MATERIAL_COMMAND:
            return "command";
        case DiagralErrorHardwareType::DIGRAL_ERROR_MATERIAL_SIREN:
            return "siren";
        default:
            return "unknown";
        }
    }
    std::string DiagralDetectionEventTypeToString(DiagralDetectionEventType event)
    {
        switch (event)
        {
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_DISSUASION:
            return "dissuasion";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_PRE_ALARM:
            return "pre_alarm";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_CAUTION:
            return "caution";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_INTRUSION:
            return "intrusion";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_TIMER_START:
            return "timer_start";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_TIMER_END:
            return "timer_end";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_PRE_ALARM_CONFIRMED:
            return "pre_alarm_confirmed";
        case DiagralDetectionEventType::DIAGRAL_DETECTION_EVENT_INTRUSION_CONFIRMED:
            return "intrusion_confirmed";
        default:
            return "unknown";
        }
    }
    std::string DiagralSensorTypeToString(DiagralDetectionSensorType sensor)
    {
        switch (sensor)
        {
        case DiagralDetectionSensorType::DIAGRAL_SENSOR_TIMEOUT:
            return "timeout";
        case DiagralDetectionSensorType::DIAGRAL_SENSOR_MOVEMENT:
            return "movement";
        case DiagralDetectionSensorType::DIAGRAL_SENSOR_OPENING:
            return "opening";
        default:
            return "unknown";
        }
    }
}