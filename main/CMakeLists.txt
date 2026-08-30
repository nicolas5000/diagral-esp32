idf_component_register(SRCS "main.cpp" "DiagralManager.cpp" "../diagral/DiagralController.cpp" "../diagral/DiagralCommands.cpp" "../diagral/DiagralDeviceState.cpp"
                        "../helpers/NvsHelpers.cpp" "../helpers/NetworkHelpers.cpp" "../helpers/MqttHelpers.cpp" "../helpers/SyslogHelpers.cpp"
                        "../helpers/CmdLineManagement.cpp" "../helpers/CmdLineDiagralTools.cpp" "../helpers/CmdLineMqttTools.cpp" "../helpers/CmdLineNetworkTools.cpp"
                        "../helpers/CmdLineSyslogTools.cpp"
                        "../config/MiscConfig.cpp" "../config/NetworkConfig.cpp" "../config/HardwareConfig.cpp" "../config/MqttConfig.cpp" "../config/DiagralConfig.cpp"
                        "../config/SyslogConfig.cpp"
                    INCLUDE_DIRS "." "../config" "../helpers" "../diagral"
                    PRIV_REQUIRES esp_adc
                    REQUIRES esp_timer esp_driver_spi esp_driver_gpio console esp_driver_uart mbedtls nvs_flash esp_wifi esp_eth mqtt esp_app_format littlefs)