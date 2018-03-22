/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__


#include <stdint.h>
#include <vector>


#include "rapidjson/fwd.h"


class Addr;
class Url;


namespace xmrig {


class ConfigLoader;
class IWatcherListener;


/**
 * @brief The Config class
 *
 * Options with dynamic reload:
 *   colors
 *   debug
 *   verbose
 *   custom-diff (only for new connections)
 *   api/worker-id
 *   pools/
 */
class Config
{
    friend class ConfigLoader;

public:
    enum Algorithm {
        CRYPTONIGHT,      /* CryptoNight (Monero) */
        CRYPTONIGHT_LITE, /* CryptoNight-Lite (AEON) */
    };

    enum Mode {
        NICEHASH_MODE,
        SIMPLE_MODE
    };

    Config();
    ~Config();

    bool isValid() const;
    bool reload(const char *json);
    bool save();
    const char *algoName() const;
    const char *modeName() const;
    void getJSON(rapidjson::Document &doc);

    static Config *load(int argc, char **argv, IWatcherListener *listener);

    inline bool apiIPv6() const                    { return m_apiIPv6; }
    inline bool apiRestricted() const              { return m_apiRestricted; }
    inline bool background() const                 { return m_background; }
    inline bool colors() const                     { return m_colors; }
    inline bool isDebug() const                    { return m_debug; }
    inline bool syslog() const                     { return m_syslog; }
    inline bool verbose() const                    { return m_verbose; }
    inline bool watch() const                      { return m_watch && m_fileName; }
    inline bool workers() const                    { return m_workers; }
    inline const char *accessLog() const           { return m_accessLog; }
    inline const char *apiToken() const            { return m_apiToken; }
    inline const char *apiWorkerId() const         { return m_apiWorkerId; }
    inline const char *fileName() const            { return m_fileName; }
    inline const char *logFile() const             { return m_logFile; }
    inline const char *userAgent() const           { return m_userAgent; }
    inline const std::vector<Addr*> &addrs() const { return m_addrs; }
    inline const std::vector<Url*> &pools() const  { return m_pools; }
    inline int algorithm() const                   { return m_algorithm; }
    inline int apiPort() const                     { return m_apiPort; }
    inline int donateLevel() const                 { return m_donateLevel; }
    inline int mode() const                        { return m_mode; }
    inline int retries() const                     { return m_retries; }
    inline int retryPause() const                  { return m_retryPause; }
    inline int reuseTimeout() const                { return m_reuseTimeout; }
    inline uint64_t diff() const                   { return m_diff; }
    inline void setColors(bool colors)             { m_colors = colors; }
    inline void setVerbose(bool verbose)           { m_verbose = verbose; }
    inline void toggleVerbose()                    { m_verbose = !m_verbose; }

private:
    void adjust();
    void setAlgo(const char *algo);
    void setCoin(const char *coin);
    void setFileName(const char *fileName);
    void setMode(const char *mode);

    bool m_adjusted;
    bool m_apiIPv6;
    bool m_apiRestricted;
    bool m_background;
    bool m_colors;
    bool m_debug;
    bool m_ready;
    bool m_syslog;
    bool m_verbose;
    bool m_watch;
    bool m_workers;
    char *m_accessLog;
    char *m_apiToken;
    char *m_apiWorkerId;
    char *m_fileName;
    char *m_logFile;
    char *m_userAgent;
    int m_algorithm;
    int m_apiPort;
    int m_donateLevel;
    int m_mode;
    int m_retries;
    int m_retryPause;
    int m_reuseTimeout;
    std::vector<Addr*> m_addrs;
    std::vector<Url*> m_pools;
    uint64_t m_diff;
};


} /* namespace xmrig */

#endif /* __CONFIG_H__ */
