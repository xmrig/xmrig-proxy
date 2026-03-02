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

#include "core/config/Config.h"
#include "3rdparty/rapidjson/document.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IJsonReader.h"
#include "base/net/dns/Dns.h"
#include "donate.h"


#include <array>
#include <cassert>
#include <climits>
#include <cstring>
#include <uv.h>


namespace xmrig {


static const std::array<const char *, 3> modeNames = { "nicehash", "simple", "extra_nonce"};


} // namespace xmrig


#if defined(_WIN32) && !defined(strncasecmp)
#   define strncasecmp _strnicmp
#endif


const char *xmrig::Config::modeName() const
{
    return modeNames[m_mode];
}


bool xmrig::Config::isVerbose() const
{
    return Log::isVerbose();
}


bool xmrig::Config::read(const IJsonReader &reader, const char *fileName)
{
    if (!BaseConfig::read(reader, fileName)) {
        return false;
    }

    m_customDiffStats = reader.getBool("custom-diff-stats", m_customDiffStats);
    m_debug        = reader.getBool("debug", m_debug);
    m_algoExt      = reader.getBool("algo-ext", m_algoExt);
    m_reuseTimeout = reader.getInt("reuse-timeout", m_reuseTimeout);
    m_accessLog    = reader.getString("access-log-file");
    m_password     = reader.getString("access-password");

    setCustomDiff(reader.getUint64("custom-diff", m_diff));
    setMode(reader.getString("mode"));
    setWorkersMode(reader.getValue("workers"));

    const rapidjson::Value &bind = reader.getArray("bind");
    if (bind.IsArray()) {
        for (const rapidjson::Value &value : bind.GetArray()) {
            if (value.IsObject()) {
                BindHost host(value);
                if (host.isValid()) {
                    m_bind.push_back(std::move(host));
                }
            }
            else if (value.IsString()) {
                BindHost host(value.GetString());
                if (host.isValid()) {
                    m_bind.push_back(std::move(host));
                }
            }
        }
    }

    if (m_bind.empty()) {
        m_bind.push_back(BindHost("0.0.0.0", 3333, 4));
        m_bind.push_back(BindHost("::", 3333, 6));
    }

    return true;
}


void xmrig::Config::getJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;

    doc.SetObject();

    auto &allocator = doc.GetAllocator();

    doc.AddMember("access-log-file",                m_accessLog.toJSON(), allocator);
    doc.AddMember("access-password",                m_password.toJSON(), allocator);
    doc.AddMember("algo-ext",                       m_algoExt, allocator);

    Value api(kObjectType);
    api.AddMember(StringRef(kApiId),                m_apiId.toJSON(), allocator);
    api.AddMember(StringRef(kApiWorkerId),          m_apiWorkerId.toJSON(), allocator);
    doc.AddMember(StringRef(kApi),                  api, allocator);
    doc.AddMember(StringRef(kHttp),                 m_http.toJSON(doc), allocator);

    doc.AddMember(StringRef(kBackground),           isBackground(), allocator);

    Value bind(kArrayType);
    for (const auto &host : m_bind) {
        bind.PushBack(host.toJSON(doc), allocator);
    }

    doc.AddMember("bind",                           bind, allocator);
    doc.AddMember(StringRef(kColors),               Log::isColors(), allocator);
    doc.AddMember("custom-diff",                    diff(), allocator);
    doc.AddMember("custom-diff-stats",              m_customDiffStats, allocator);
    doc.AddMember(StringRef(Pools::kDonateLevel),   m_pools.donateLevel(), allocator);
    doc.AddMember(StringRef(kLogFile),              m_logFile.toJSON(), allocator);
    doc.AddMember("mode",                           StringRef(modeName()), allocator);
    doc.AddMember(StringRef(Pools::kPools),         m_pools.toJSON(doc), allocator);
    doc.AddMember(StringRef(Pools::kRetries),       m_pools.retries(), allocator);
    doc.AddMember(StringRef(Pools::kRetryPause),    m_pools.retryPause(), allocator);
    doc.AddMember("reuse-timeout",                  reuseTimeout(), allocator);

#   ifdef XMRIG_FEATURE_TLS
    doc.AddMember(StringRef(kTls),                  m_tls.toJSON(doc), allocator);
#   endif

    doc.AddMember(StringRef(DnsConfig::kField),     Dns::config().toJSON(doc), allocator);
    doc.AddMember(StringRef(kUserAgent),            m_userAgent.toJSON(), allocator);
    doc.AddMember(StringRef(kSyslog),               isSyslog(), allocator);
    doc.AddMember(StringRef(kVerbose),              isVerbose(), allocator);
    doc.AddMember(StringRef(kWatch),                m_watch,     allocator);
    doc.AddMember("workers",                        Workers::modeToJSON(workersMode()), allocator);
}


void xmrig::Config::toggleVerbose()
{
    Log::setVerbose(Log::isVerbose() ? 0 : 1);
}


void xmrig::Config::setCustomDiff(uint64_t diff)
{
    if (diff >= 100 && diff < INT_MAX) {
        m_diff = diff;
    }
}


void xmrig::Config::setMode(const char *mode)
{
    if (mode == nullptr) {
        m_mode = NICEHASH_MODE;
        return;
    }

    for (size_t i = 0; i < modeNames.size(); i++) {
        if (modeNames[i] && !strcmp(mode, modeNames[i])) {
            m_mode = static_cast<Mode>(i);
            break;
        }
    }
}


void xmrig::Config::setWorkersMode(const rapidjson::Value &value)
{
    if (value.IsBool()) {
        m_workersMode = value.GetBool() ? Workers::RigID : Workers::None;
    }
    else if (value.IsString()) {
        m_workersMode = Workers::parseMode(value.GetString());
    }
}
