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

#ifndef XMRIG_CONFIG_H
#define XMRIG_CONFIG_H


#include "3rdparty/rapidjson/fwd.h"
#include "base/kernel/config/BaseConfig.h"
#include "base/tools/String.h"
#include "proxy/BindHost.h"
#include "proxy/workers/Workers.h"


#include <cstdint>
#include <vector>


namespace xmrig {


class ConfigLoader;
class IConfigListener;
class Process;


class Config : public BaseConfig
{
public:
    enum Mode {
        NICEHASH_MODE,
        SIMPLE_MODE,
        EXTRA_NONCE_MODE,
    };

    Config() = default;

    const char *modeName() const;

    bool isVerbose() const;
    bool read(const IJsonReader &reader, const char *fileName) override;
    void getJSON(rapidjson::Document &doc) const override;
    void toggleVerbose();

    inline bool hasAlgoExt() const                 { return isDonateOverProxy() ? m_algoExt : true; }
    inline bool isCustomDiffStats() const          { return m_customDiffStats; }
    inline bool isDebug() const                    { return m_debug; }
    inline bool isDonateOverProxy() const          { return m_pools.donateLevel() == 0 || m_mode == SIMPLE_MODE; }
    inline bool isShouldSave() const               { return m_upgrade && isAutoSave(); }
    inline const BindHosts &bind() const           { return m_bind; }
    inline const String &accessLog() const         { return m_accessLog; }
    inline const String &password() const          { return m_password; }
    inline int mode() const                        { return m_mode; }
    inline int reuseTimeout() const                { return m_reuseTimeout; }
    inline static IConfig *create()                { return new Config(); }
    inline uint64_t diff() const                   { return m_diff; }
    inline Workers::Mode workersMode() const       { return m_workersMode; }

private:
    void setCustomDiff(uint64_t diff);
    void setMode(const char *mode);
    void setWorkersMode(const rapidjson::Value &value);

    BindHosts m_bind;
    bool m_algoExt              = true;
    bool m_customDiffStats      = false;
    bool m_debug                = false;
    int m_mode                  = NICEHASH_MODE;
    int m_reuseTimeout          = 0;
    String m_accessLog;
    String m_password;
    uint64_t m_diff             = 0;
    Workers::Mode m_workersMode = Workers::RigID;
};


} /* namespace xmrig */

#endif /* XMRIG_CONFIG_H */
