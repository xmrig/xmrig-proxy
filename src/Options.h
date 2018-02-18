/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2018 XMRig       <support@xmrig.com>
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

#ifndef __OPTIONS_H__
#define __OPTIONS_H__


#include <stdint.h>
#include <vector>


#include "proxy/Addr.h"
#include "rapidjson/fwd.h"


class Url;
struct option;


class Options
{
public:
    static inline Options* i() { return m_self; }
    static Options *parse(int argc, char **argv);

    inline bool background() const                 { return m_background; }
    inline bool colors() const                     { return m_colors; }
    inline bool isDebug() const                    { return m_debug; }
    inline bool syslog() const                     { return m_syslog; }
    inline bool verbose() const                    { return m_verbose; }
    inline bool workers() const                    { return m_workers; }
    inline const char *accessLog() const           { return m_accessLog; }
    inline const char *apiToken() const            { return m_apiToken; }
    inline const char *apiWorkerId() const         { return m_apiWorkerId; }
    inline const char *coin() const                { return m_userAgent; }
    inline const char *logFile() const             { return m_logFile; }
    inline const char *userAgent() const           { return m_userAgent; }
    inline const std::vector<Addr*> &addrs() const { return m_addrs; }
    inline const std::vector<Url*> &pools() const  { return m_pools; }
    inline int apiPort() const                     { return m_apiPort; }
    inline int donateLevel() const                 { return m_donateLevel; }
    inline int retries() const                     { return m_retries; }
    inline int retryPause() const                  { return m_retryPause; }
    inline uint64_t diff() const                   { return m_diff; }
    inline void setVerbose(bool verbose)           { m_verbose = verbose; }
    inline void toggleVerbose()                    { m_verbose = !m_verbose; }
    inline void setColors(bool colors)             { m_colors = colors; }

    inline static void release()                   { delete m_self; }

private:
    Options(int argc, char **argv);
    ~Options();

    inline bool isReady() const { return m_ready; }

    static Options *m_self;

    bool getJSON(const char *fileName, rapidjson::Document &doc);
    bool parseArg(int key, const char *arg);
    bool parseArg(int key, uint64_t arg);
    bool parseBoolean(int key, bool enable);
    Url *parseUrl(const char *arg) const;
    void parseConfig(const char *fileName);
    void parseJSON(const struct option *option, const rapidjson::Value &object);
    void showUsage(int status) const;
    void showVersion(void);

    bool m_background;
    bool m_colors;
    bool m_debug;
    bool m_ready;
    bool m_syslog;
    bool m_verbose;
    bool m_workers;
    char *m_accessLog;
    char *m_apiToken;
    char *m_apiWorkerId;
    char *m_coin;
    char *m_logFile;
    char *m_userAgent;
    int m_apiPort;
    int m_donateLevel;
    int m_retries;
    int m_retryPause;
    std::vector<Addr*> m_addrs;
    std::vector<Url*> m_pools;
    uint64_t m_diff;
};

#endif /* __OPTIONS_H__ */
