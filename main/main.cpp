#include <stdio.h>
#include <string.h>

#include "HardwareConfig.hpp"
#include "DiagralManager.hpp"
#include "CmdLineManagement.hpp"

#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_console.h"

extern "C" void app_main(void)
{
    // Initialize Hardware: NVS, LittleFS, GPIO ISR, SPI bus
    esp_err_t err = Config::InitHardware();
    ESP_ERROR_CHECK(err);

    // Initialize Manager
    Diagral::DiagralManager diagralManager = Diagral::DiagralManager();

    vTaskDelay(pdMS_TO_TICKS(10000));
    // Initialize commands line tools
    init_cmdline(&diagralManager);

    while (true)
        vTaskDelay(pdMS_TO_TICKS(60000));
}