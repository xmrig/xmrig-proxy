/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_CONFIG_H
#define XMRIG_CONFIG_H


#include <stdint.h>
#include <vector>


#include "common/config/CommonConfig.h"
#include "common/utils/c_str.h"
#include "proxy/Addr.h"
#include "proxy/workers/Workers.h"
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
class Config : public CommonConfig
{
public:
    enum Mode {
        NICEHASH_MODE,
        SIMPLE_MODE
    };

    Config();

    bool reload(const char *json);

    const char *modeName() const;
    void getJSON(rapidjson::Document &doc) const override;

    static Config *load(int argc, char **argv, IWatcherListener *listener);

    inline bool isDebug() const                    { return m_debug; }
    inline bool isVerbose() const                  { return m_verbose; }
    inline const char *accessLog() const           { return m_accessLog.data(); }
    inline const std::vector<Addr> &addrs() const  { return m_addrs; }
    inline int mode() const                        { return m_mode; }
    inline int reuseTimeout() const                { return m_reuseTimeout; }
    inline uint64_t diff() const                   { return m_diff; }
    inline void setColors(bool colors)             { m_colors = colors; }
    inline void setVerbose(bool verbose)           { m_verbose = verbose; }
    inline void toggleVerbose()                    { m_verbose = !m_verbose; }
    inline Workers::Mode workersMode() const       { return m_workersMode; }

protected:
    bool finalize() override;
    bool parseBoolean(int key, bool enable) override;
    bool parseString(int key, const char *arg) override;
    bool parseUint64(int key, uint64_t arg) override;
    void parseJSON(const rapidjson::Document &doc) override;

private:
    void setMode(const char *mode);

    bool m_debug;
    bool m_ready;
    bool m_verbose;

    int m_mode;
    int m_reuseTimeout;
    std::vector<Addr> m_addrs;
    uint64_t m_diff;
    Workers::Mode m_workersMode;
    xmrig::c_str m_accessLog;
};


} /* namespace xmrig */

#endif /* XMRIG_CONFIG_H */
