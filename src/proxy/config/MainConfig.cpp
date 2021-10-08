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

#include "proxy/config/MainConfig.h"
#include "3rdparty/rapidjson/document.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IJsonReader.h"
#include "base/tools/Arguments.h"
#include "donate.h"


#include <array>
#include <cassert>
#include <climits>
#include <cstring>
#include <uv.h>


namespace xmrig {


static const std::array<const char *, 3> modeNames = { "nicehash", "simple", "extra_nonce" };
static const char *kAlgoExt     = "algo-ext";
static const char *kBind        = "bind";
static const char *kMode        = "mode";
static const char *kTls         = "tls";


} // namespace xmrig


#if defined(_WIN32) && !defined(strncasecmp)
#   define strncasecmp _strnicmp
#endif


xmrig::MainConfig::MainConfig(const Arguments &arguments)
{
    m_algoExt = !arguments.contains("--no-algo-ext");
}


xmrig::MainConfig::MainConfig(const IJsonReader &reader, const MainConfig &current)
{
    setMode(reader.getString(kMode), current.m_mode);
    m_pools.load(reader);

#   ifdef XMRIG_FEATURE_TLS
    m_tls = reader.getValue(kTls);
#   endif

    m_algoExt = reader.getBool(kAlgoExt, current.m_algoExt);

    const auto &bind = reader.getArray(kBind);
    if (bind.IsArray()) {
        for (const auto &value : bind.GetArray()) {
            if (value.IsObject()) {
                BindHost host(value);
                if (host.isValid()) {
                    m_bind.emplace_back(std::move(host));
                }
            }
            else if (value.IsString()) {
                BindHost host(value.GetString());
                if (host.isValid()) {
                    m_bind.emplace_back(std::move(host));
                }
            }
        }
    }

    if (m_bind.empty()) {
        if (current.bind().empty()) {
            m_bind.emplace_back("0.0.0.0", 3333, 4);
            m_bind.emplace_back("::", 3333, 6);
        }
        else {
            m_bind = current.bind();
        }
    }
}


const char *xmrig::MainConfig::modeName() const
{
    return modeNames[m_mode];
}


void xmrig::MainConfig::save(rapidjson::Document &doc) const
{
    using namespace rapidjson;

    auto &allocator = doc.GetAllocator();

    doc.AddMember(StringRef(kAlgoExt),              m_algoExt ? Value(kNullType) : Value(m_algoExt), allocator);

    Value bind(kArrayType);
    for (const auto &host : m_bind) {
        bind.PushBack(host.toJSON(doc), allocator);
    }

    doc.AddMember(StringRef(kBind),                 bind, allocator);
    doc.AddMember(StringRef(Pools::kDonateLevel),   m_pools.donateLevel(), allocator);
    doc.AddMember(StringRef(kMode),                 m_mode == NICEHASH_MODE ? Value(kNullType) : Value(StringRef(modeName())), allocator);
    doc.AddMember(StringRef(Pools::kPools),         m_pools.toJSON(doc), allocator);
    doc.AddMember(StringRef(Pools::kRetries),       m_pools.retries(), allocator);
    doc.AddMember(StringRef(Pools::kRetryPause),    m_pools.retryPause(), allocator);

#   ifdef XMRIG_FEATURE_TLS
    doc.AddMember(StringRef(kTls),                  m_tls.toJSON(doc), allocator);
#   endif
}


void xmrig::MainConfig::setMode(const char *mode, int current)
{
    if (mode == nullptr) {
        m_mode = current;
        return;
    }

    for (size_t i = 0; i < modeNames.size(); i++) {
        if (modeNames[i] && !strcmp(mode, modeNames[i])) {
            m_mode = static_cast<Mode>(i);
            break;
        }
    }
}
