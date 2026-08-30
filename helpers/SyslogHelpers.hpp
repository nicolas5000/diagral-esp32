#pragma once

#include "esp_log.h"

#include <string>

namespace Helpers
{
    class SyslogHelpers
    {
    public:
        /// @brief Initialise the syslog client. Call once.
        SyslogHelpers();

        /// @brief Send one pre-formatted ESP log line to the syslog server.
        /// @param log_level ESP log level.
        /// @param tag ESP log TAG.
        /// @param line ESP log line.
        void Send(esp_log_level_t log_level, const char *tag, std::string log);

    private:
        int mSock;
        // Cached config — to avoid NVS reads per line
        bool mEnabled;
        uint8_t mFacility;
        uint8_t mMinLevel;
        std::string mServer;
        uint16_t mPort;
        std::string mHostname;
    };
}
