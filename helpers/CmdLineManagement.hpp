#pragma once

#include "DiagralManager.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Initialize command line
    /// @param diagral_manager Pointer to DiagralManager object
    void init_cmdline(Diagral::DiagralManager *diagral_manager);

    /// @brief Initialize command line tools
    void init_cmdline_tools();

    /// @brief Register misc. command line tools (like reboot)
    void register_misc_cmdline_tools();

    /// @brief Register network configuration command line tools (Wifi, DHCP, static IPv4, DNS and SNTP configuration)
    void register_network_config_cmdline_tools();

    /// @brief Register MQTT configuration command line tools
    void register_mqtt_config_cmdline_tools();

    /// @brief Register Syslog configuration command line tools
    void register_syslog_config_cmdline_tools();

    /// @brief Register command line tools
    /// @param diagral_manager Pointer to DiagralManager object
     void register_diagral_cmdline_tools(Diagral::DiagralManager *diagral_manager);

    /// @brief Initialize hardware console (driver, ...)
    void init_console();

#ifdef __cplusplus
}
#endif