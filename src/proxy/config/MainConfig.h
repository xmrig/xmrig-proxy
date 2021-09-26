/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_MAINCONFIG_H
#define XMRIG_MAINCONFIG_H


#include "base/kernel/interfaces/IJsonReader.h"
#include "base/net/stratum/Pools.h"
#include "base/tools/String.h"
#include "proxy/BindHost.h"


#ifdef XMRIG_FEATURE_TLS
#   include "base/net/tls/TlsConfig.h"
#endif


#include <cstdint>
#include <vector>


namespace xmrig {


class Arguments;
class ConfigLoader;
class IConfigListener;
class Process;


class MainConfig
{
public:
    enum Mode {
        NICEHASH_MODE,
        SIMPLE_MODE,
        EXTRA_NONCE_MODE,
    };

    MainConfig() = default;
    MainConfig(const Arguments &arguments);
    MainConfig(const IJsonReader &reader, const MainConfig &current);

    const char *modeName() const;

    bool read(const IJsonReader &reader, const char *fileName);
    void save(rapidjson::Document &doc) const;

    inline bool hasAlgoExt() const                  { return isDonateOverProxy() ? m_algoExt : true; }
    inline bool isAutoSave() const                  { return m_autoSave; }
    inline bool isDonateOverProxy() const           { return m_pools.donateLevel() == 0 || m_mode == SIMPLE_MODE; }
    inline bool isShouldSave() const                { return m_upgrade && isAutoSave(); }
    inline const BindHosts &bind() const            { return m_bind; }
    inline const Pools &pools() const               { return m_pools; }
    inline int mode() const                         { return m_mode; }

#   ifdef XMRIG_FEATURE_TLS
    inline const TlsConfig &tls() const             { return m_tls; }
#   endif

private:
    void setMode(const char *mode, int current);

    BindHosts m_bind;
    bool m_algoExt              = true;
    bool m_autoSave             = true;
    bool m_upgrade              = false;
    int m_mode                  = NICEHASH_MODE;
    Pools m_pools;

#   ifdef XMRIG_FEATURE_TLS
    TlsConfig m_tls;
#   endif
};


} // namespace xmrig


#endif // XMRIG_MAINCONFIG_H
