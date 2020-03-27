/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "base/tools/String.h"
#include "common/config/CommonConfig.h"
#include "proxy/BindHost.h"
#include "proxy/workers/Workers.h"
#include "rapidjson/fwd.h"


#ifdef XMRIG_FEATURE_TLS
#   include "proxy/tls/TlsConfig.h"
#endif


namespace xmrig {


class ConfigLoader;
class IConfigListener;
class Process;


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

    bool isTLS() const;
    bool reload(const rapidjson::Value &json);
    const char *modeName() const;

    void getJSON(rapidjson::Document &doc) const override;

    static Config *load(Process *process, IConfigListener *listener);

    inline bool hasAlgoExt() const                 { return isDonateOverProxy() ? m_algoExt : true; }
    inline bool isDebug() const                    { return m_debug; }
    inline bool isDonateOverProxy() const          { return m_pools.donateLevel() == 0 || m_mode == SIMPLE_MODE; }
    inline bool isShouldSave() const               { return m_upgrade && isAutoSave(); }
    inline bool isVerbose() const                  { return m_verbose; }
    inline const String &accessLog() const         { return m_accessLog; }
    inline const String &password() const          { return m_password; }
    inline const xmrig::BindHosts &bind() const    { return m_bind; }
    inline int mode() const                        { return m_mode; }
    inline int reuseTimeout() const                { return m_reuseTimeout; }
    inline static IConfig *create()                { return new Config(); }
    inline uint64_t diff() const                   { return m_diff; }
    inline void setVerbose(bool verbose)           { m_verbose = verbose; }
    inline void toggleVerbose()                    { m_verbose = !m_verbose; }
    inline Workers::Mode workersMode() const       { return m_workersMode; }

#   ifdef XMRIG_FEATURE_TLS
    inline const xmrig::TlsConfig &tls() const { return m_tls; }
#   endif

protected:
    bool finalize() override;
    bool parseBoolean(int key, bool enable) override;
    bool parseString(int key, const char *arg) override;
    bool parseUint64(int key, uint64_t arg) override;
    void parseJSON(const rapidjson::Value &json) override;

private:
    void setMode(const char *mode);

    BindHosts m_bind;
    bool m_algoExt;
    bool m_debug;
    bool m_verbose;
    int m_mode;
    int m_reuseTimeout;
    String m_accessLog;
    String m_password;
    uint64_t m_diff;
    Workers::Mode m_workersMode;

#   ifdef XMRIG_FEATURE_TLS
    TlsConfig m_tls;
#   endif
};


} /* namespace xmrig */

#endif /* XMRIG_CONFIG_H */
