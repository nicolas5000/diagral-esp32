#include "CmdLineManagement.hpp"
#include "DiagralConfig.hpp"

#include <algorithm>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include <esp_timer.h>

using namespace Config;

static const char *TAG = "cmdline_mngt";
static Diagral::DiagralManager *sDiagralManager;
static Diagral::DiagralController *sDiagralController;

// ******************* DIAGRAL GET STATE ********************

static int do_diagral_get_state_cmd(int argc, char **argv)
{
    if (!sDiagralController->ForceDeviceStateUpdate())
    {
        ESP_LOGW(TAG, "Update failed");
    }
    return 0;
}

static void register_diagral_get_state(void)
{
    const esp_console_cmd_t diagral_get_state_cmd = {
        .command = "diagral_get_state",
        .help = "Get current state from Diagral system",
        .hint = NULL,
        .func = &do_diagral_get_state_cmd,
        .argtable = NULL,
        .func_w_context = NULL,
        .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_get_state_cmd));
}

// ******************* DIAGRAL ARM ALL ********************

static int do_diagral_arm_all_cmd(int argc, char **argv)
{
    if (!sDiagralController->ArmAway(Diagral::DIAGRAL_DATA_ZONES_ALL))
    {
        ESP_LOGW(TAG, "Arm all zones failed");
    }
    return 0;
}

static void register_diagral_arm_all(void)
{
    const esp_console_cmd_t diagral_arm_all_cmd = {
        .command = "diagral_arm_all",
        .help = "Arm all zones in Diagral system",
        .hint = NULL,
        .func = &do_diagral_arm_all_cmd,
        .argtable = NULL,
        .func_w_context = NULL,
        .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_arm_all_cmd));
}

// ******************* DIAGRAL ARM ZONES ********************

/// @brief Structure used by the 'diagral_arm_zones' command
static struct
{
    struct arg_int *zones;
    struct arg_end *end;
} diagral_arm_zones_args;

static int do_diagral_arm_zones_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&diagral_arm_zones_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, diagral_arm_zones_args.end, argv[0]);
        return 1;
    }
    if (diagral_arm_zones_args.zones->ival[0] >= 0 && diagral_arm_zones_args.zones->ival[0] <= Diagral::DIAGRAL_DATA_ZONES_ALL)
    {
        sDiagralController->ArmAway(diagral_arm_zones_args.zones->ival[0]);
    }
    else
        ESP_LOGE(TAG, "Invalid value for <zones>");
    return 0;
}

void register_diagral_arm_zones(void)
{
    diagral_arm_zones_args.zones = arg_int1(NULL, NULL, "<zones>", "Specify the zone(s) to arm as a bit field (1 for zone 1, 2 for zone 2, 4 for zone 3, 8 for zone 4, 3 for zones 1 and 2, ...)");
    diagral_arm_zones_args.end = arg_end(1);

    const esp_console_cmd_t diagral_arm_zones_cmd = {
        .command = "diagral_arm_zones",
        .help = "Arm specified zones in Diagral system",
        .hint = NULL,
        .func = &do_diagral_arm_zones_cmd,
        .argtable = &diagral_arm_zones_args,
        .func_w_context = NULL,
        .context = NULL};

    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_arm_zones_cmd));
}

// ******************* DIAGRAL ARM HOME ********************

static int do_diagral_arm_home_cmd(int argc, char **argv)
{
    if (!sDiagralController->ArmHome())
    {
        ESP_LOGW(TAG, "Arm home failed");
    }
    return 0;
}

static void register_diagral_arm_home(void)
{
    const esp_console_cmd_t diagral_arm_home_cmd = {
        .command = "diagral_arm_home",
        .help = "Arm 'Home presence' in Diagral system",
        .hint = NULL,
        .func = &do_diagral_arm_home_cmd,
        .argtable = NULL,
        .func_w_context = NULL,
        .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_arm_home_cmd));
}

// ******************* DIAGRAL DISARM ALL ********************

static int do_diagral_disarm_all_cmd(int argc, char **argv)
{
    if (!sDiagralController->Disarm(Diagral::DIAGRAL_DATA_ZONES_ALL))
    {
        ESP_LOGW(TAG, "Disarm all zones failed");
    }
    return 0;
}

static void register_diagral_disarm_all(void)
{
    const esp_console_cmd_t diagral_disarm_all_cmd = {
        .command = "diagral_disarm_all",
        .help = "Disarm all zones in Diagral system",
        .hint = NULL,
        .func = &do_diagral_disarm_all_cmd,
        .argtable = NULL,
        .func_w_context = NULL,
        .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_disarm_all_cmd));
}

// ******************* DIAGRAL DISARM ZONES ********************

/// @brief Structure used by the 'diagral_disarm_zones' command
static struct
{
    struct arg_int *zones;
    struct arg_end *end;
} diagral_disarm_zones_args;

static int do_diagral_disarm_zones_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&diagral_disarm_zones_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, diagral_disarm_zones_args.end, argv[0]);
        return 1;
    }
    if (diagral_disarm_zones_args.zones->ival[0] >= 0 && diagral_disarm_zones_args.zones->ival[0] <= Diagral::DIAGRAL_DATA_ZONES_ALL)
    {
        sDiagralController->Disarm(diagral_disarm_zones_args.zones->ival[0]);
    }
    else
        ESP_LOGE(TAG, "Invalid value for <zones>");
    return 0;
}

void register_diagral_disarm_zones(void)
{
    diagral_disarm_zones_args.zones = arg_int1(NULL, NULL, "<zones>", "Specify the zone(s) to disarm as a bit field (1 for zone 1, 2 for zone 2, 4 for zone 3, 8 for zone 4, 3 for zones 1 and 2, ...)");
    diagral_disarm_zones_args.end = arg_end(1);

    const esp_console_cmd_t diagral_disarm_zones_cmd = {
        .command = "diagral_disarm_zones",
        .help = "Disarm specified zones in Diagral system",
        .hint = NULL,
        .func = &do_diagral_disarm_zones_cmd,
        .argtable = &diagral_disarm_zones_args,
        .func_w_context = NULL,
        .context = NULL};

    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_disarm_zones_cmd));
}

// ******************* DIAGRAL CHECK PIN ********************

/// @brief Structure used by the 'diagral_checkpin' command
static struct
{
    struct arg_str *pin;
    struct arg_end *end;
} diagral_checkpin_args;

static int do_checkpin_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&diagral_checkpin_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, diagral_checkpin_args.end, argv[0]);
        return 1;
    }
    size_t pin_length = strlen(diagral_checkpin_args.pin->sval[0]);
    if (pin_length < 4 || pin_length > 6)
    {
        ESP_LOGE(TAG, "PIN code must be 4 to 6 digits!");
        return 1;
    }
    if (sDiagralController->CheckPinCode(diagral_checkpin_args.pin->sval[0]))
    {
        ESP_LOGI(TAG, "PIN code is valid!");
    }
    return 0;
}

void register_diagral_checkpin(void)
{
    diagral_checkpin_args.pin = arg_str1(NULL, NULL, "<pin code>", "PIN code, 4 to 6 digits");
    diagral_checkpin_args.end = arg_end(1);

    const esp_console_cmd_t checkpin_cmd = {
        .command = "diagral_checkpin",
        .help = "Check a PIN code in Diagral system",
        .hint = NULL,
        .func = &do_checkpin_cmd,
        .argtable = &diagral_checkpin_args,
        .func_w_context = NULL,
        .context = NULL};

    ESP_ERROR_CHECK(esp_console_cmd_register(&checkpin_cmd));
}

// ******************* DIAGRAL SEND RAW ********************

/// @brief Structure used by the 'diagral_sendraw' command
static struct
{
    struct arg_str *raw_frame;
    struct arg_end *end;
} diagral_sendraw_args;

static int do_sendraw_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&diagral_sendraw_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, diagral_sendraw_args.end, argv[0]);
        return 1;
    }
    sDiagralController->SendRaw(diagral_sendraw_args.raw_frame->sval[0]);
    return 0;
}

void register_diagral_sendraw(void)
{
    diagral_sendraw_args.raw_frame = arg_str1(NULL, NULL, "<raw frame>", "String representation of the DiagralFrame, from Identifier byte to last byte of data (without Length, counter and CRC)");
    diagral_sendraw_args.end = arg_end(1);

    const esp_console_cmd_t sendraw_cmd = {
        .command = "diagral_sendraw",
        .help = "Send an Digral frame from given string representation, waits for ACK.",
        .hint = NULL,
        .func = &do_sendraw_cmd,
        .argtable = &diagral_sendraw_args,
        .func_w_context = NULL,
        .context = NULL};

    ESP_ERROR_CHECK(esp_console_cmd_register(&sendraw_cmd));
}

// ******************* DIAGRAL CONFIG ********************

/// @brief Structure used by the 'diagral_config' command
static struct
{
    struct arg_lit *read;
    struct arg_lit *del;
    struct arg_int *logging_state;
    struct arg_int *passive_state;
    struct arg_int *pin_code_check;
    struct arg_end *end;
} diagral_config_args;

static int do_diagral_config_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&diagral_config_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, diagral_config_args.end, argv[0]);
        return 1;
    }
    if (diagral_config_args.read->count > 0)
    {
        // Read Diagral layer configuration
        ESP_LOGI(TAG, "Logging status: %s", DiagralConfig::isLoggingEnabled() ? "enabled" : "disabled");
        ESP_LOGI(TAG, "Passive mode: %s", DiagralConfig::isPassiveModeEnabled() ? "enabled" : "disabled");
        ESP_LOGI(TAG, "PIN code check: %s", DiagralConfig::isPinCodeCheckEnabled() ? "enabled" : "disabled");
    }
    else if (diagral_config_args.del->count > 0)
    {
        DiagralConfig::DeleteDiagralConfig();
        ESP_LOGI(TAG, "Diagral configuration restored to default values. New configuration will be applied after reboot!");
    }
    else
    {
        esp_err_t err;
        // Set configuration
        if (diagral_config_args.logging_state->count > 0)
        {
            err = DiagralConfig::ActivateLogging(diagral_config_args.logging_state->ival[0] != 0);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set logging state to configuration storage! (%d)", err);
            }
            else
            {
                ESP_LOGI(TAG, "Logging state set to configuration storage: %s", DiagralConfig::isLoggingEnabled() ? "enabled" : "disabled");
            }
        }
        if (diagral_config_args.passive_state->count > 0)
        {
            err = DiagralConfig::ActivatePassiveMode(diagral_config_args.passive_state->ival[0] != 0);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set passive mode to configuration storage! (%d)", err);
            }
            else
            {
                ESP_LOGI(TAG, "Passive mode set to configuration storage: %s", DiagralConfig::isPassiveModeEnabled() ? "enabled" : "disabled");
            }
        }
        if (diagral_config_args.pin_code_check->count > 0)
        {
            err = DiagralConfig::ActivatePinCodeCheck(diagral_config_args.pin_code_check->ival[0] != 0);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set PIN code check to configuration storage! (%d)", err);
            }
            else
            {
                ESP_LOGI(TAG, "PIN code check set to configuration storage: %s", DiagralConfig::isPinCodeCheckEnabled() ? "enabled" : "disabled");
            }
        }
        ESP_LOGI(TAG, "New configuration will be applied after reboot!");
    }
    return 0;
}

void register_diagral_config(void)
{
    diagral_config_args.read = arg_lit0("r", "read", "Read current configuration from storage (no other argument required)");
    diagral_config_args.del = arg_lit0("d", "delete", "Delete current configuration in storage (no other argument required)");
    diagral_config_args.logging_state = arg_int0(NULL, "logging", "<state>", "1 to enable logging in Diagral controller layer, 0 to disable");
    diagral_config_args.passive_state = arg_int0(NULL, "passive", "<state>", "1 to enable passive state in Diagral controller layer, 0 to disable");
    diagral_config_args.pin_code_check = arg_int0(NULL, "checkpin", "<state>", "1 to enable PIN code check from MQTT");
    diagral_config_args.end = arg_end(5);

    const esp_console_cmd_t diagral_config_cmd = {
        .command = "diagral_config",
        .help = "Configure Diagral controller layer",
        .hint = NULL,
        .func = &do_diagral_config_cmd,
        .argtable = &diagral_config_args,
        .func_w_context = NULL,
        .context = NULL};

    ESP_ERROR_CHECK(esp_console_cmd_register(&diagral_config_cmd));
}

// ******************* DIAGRAL Register commands ********************

void register_diagral_cmdline_tools(Diagral::DiagralManager *diagral_manager)
{
    sDiagralManager = diagral_manager;
    sDiagralController = diagral_manager->mDiagralController;
    register_diagral_get_state();
    register_diagral_arm_all();
    register_diagral_arm_zones();
    register_diagral_arm_home();
    register_diagral_disarm_all();
    register_diagral_disarm_zones();
    register_diagral_checkpin();
    register_diagral_sendraw();
    register_diagral_config();
}
