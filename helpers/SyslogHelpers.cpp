#include "SyslogHelpers.hpp"
#include "SyslogConfig.hpp"
#include "NetworkConfig.hpp"
#include "sdkconfig.h"

#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "esp_log.h"

static const char *TAG = "syslogHelpers";

using namespace Config;

static uint8_t severity_from_esp_log_level(esp_log_level_t l)
{
    // From RFC 3164 §4.1.1
    switch (l)
    {
    case esp_log_level_t::ESP_LOG_ERROR:
        return 3; // error
    case esp_log_level_t::ESP_LOG_WARN:
        return 4; // warning
    case esp_log_level_t::ESP_LOG_INFO:
        return 6; // info
    case esp_log_level_t::ESP_LOG_DEBUG:
    case esp_log_level_t::ESP_LOG_VERBOSE:
    default:
        return 7; // debug / verbose
    }
}

namespace Helpers
{
    SyslogHelpers::SyslogHelpers()
    {
        mEnabled = SyslogConfig::isEnabled();
        mFacility = SyslogConfig::GetFacility();
        mMinLevel = SyslogConfig::GetMinLevel();
        mServer = SyslogConfig::GetServer();
        mPort = SyslogConfig::GetPort();
        mHostname = NetworkConfig::GetHostname();
        mSock = -1;
        if (!mEnabled || mServer.empty())
        {
            return;
        }
        ESP_LOGI(TAG, "Syslog ready → %s:%u (facility=%u min_level=%u)",
                 mServer.c_str(), (unsigned)mPort, (unsigned)mFacility, (unsigned)mMinLevel);
    }

    void SyslogHelpers::Send(esp_log_level_t log_level, const char *tag, std::string log)
    {
        if (!mEnabled || log.length() < 1 || strlen(tag) < 1 || log_level == esp_log_level_t::ESP_LOG_NONE)
            return;
        if (mSock < 0)
        {
            // Need to open socket!
            // Resolve hostname/IP — blocking call!
            struct addrinfo hints = {};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            char port_str[8];
            snprintf(port_str, sizeof(port_str), "%u", (unsigned)mPort);

            struct addrinfo *res = nullptr;
            if (getaddrinfo(mServer.c_str(), port_str, &hints, &res) != 0 || !res)
            {
                ESP_LOGW(TAG, "Cannot resolve syslog server '%s'", mServer.c_str());
                return;
            }
            struct sockaddr_in dest     = {};
            memcpy(&dest, res->ai_addr, sizeof(dest));
            freeaddrinfo(res);

            // Create and connect new socket.
            mSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (mSock < 0)
            {
                ESP_LOGW(TAG, "Failed to create UDP socket");
                return;
            }
            if (connect(mSock, (struct sockaddr *)&dest, sizeof(dest)) < 0)
            {
                ESP_LOGW(TAG, "UDP connect failed errno=%d", errno);
                close(mSock);
                return;
            }
        }
        if (mSock > 0)
        {
            // Socket available, send log...
            uint8_t severity = severity_from_esp_log_level(log_level);
            if (severity > mMinLevel)
                return; // below configured verbosity threshold

            uint8_t pri = (mFacility * 8) + severity;

            // Omit timestamp — let the syslog server stamp with reception time.
            // This avoids wrong timestamps when NTP hasn't synced yet.
            char buf[256];
            int len = snprintf(buf, sizeof(buf), "<%u>%s %s: %s",
                               (unsigned)pri, mHostname.c_str(), tag, log.c_str());
            if (len <= 0)
                return;
            if (len >= (int)sizeof(buf))
                len = (int)sizeof(buf) - 1;

            send(mSock, buf, (size_t)len, 0);
        }
    }
}