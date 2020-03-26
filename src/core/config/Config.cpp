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

#include "core/config/Config.h"
#include "base/io/log/Log.h"
#include "donate.h"
#include "rapidjson/document.h"
#include "base/kernel/interfaces/IJsonReader.h"


#include <array>
#include <cassert>
#include <climits>
#include <cstring>
#include <uv.h>


namespace xmrig {


static const std::array<const char *, 2> modeNames = { "nicehash", "simple" };


} // namespace xmrig


#if defined(_WIN32) && !defined(strncasecmp)
#   define strncasecmp _strnicmp
#endif


bool xmrig::Config::isTLS() const
{
#   ifdef XMRIG_FEATURE_TLS
    for (const BindHost &host : m_bind) {
        if (host.isTLS()) {
            return true;
        }
    }
#   endif

    return false;
}


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

#   ifdef XMRIG_FEATURE_TLS
    const rapidjson::Value &tls = reader.getObject("tls");
    if (tls.IsObject()) {
        m_tls = TlsConfig(tls);
    }
#   endif

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

    doc.AddMember("access-log-file", m_accessLog.toJSON(), allocator);
    doc.AddMember("access-password", m_password.toJSON(), allocator);
    doc.AddMember("algo-ext",        m_algoExt, allocator);

    Value api(kObjectType);
    api.AddMember("id",           m_apiId.toJSON(), allocator);
    api.AddMember("worker-id",    m_apiWorkerId.toJSON(), allocator);
    doc.AddMember("api",          api, allocator);
    doc.AddMember("http",         m_http.toJSON(doc), allocator);

    doc.AddMember("background",   isBackground(), allocator);

    Value bind(kArrayType);
    for (const auto &host : m_bind) {
        bind.PushBack(host.toJSON(doc), allocator);
    }

    doc.AddMember("bind",          bind, allocator);
    doc.AddMember("colors",        Log::isColors(), allocator);
    doc.AddMember("custom-diff",   diff(), allocator);
    doc.AddMember("custom-diff-stats", m_customDiffStats, allocator);
    doc.AddMember("donate-level",  m_pools.donateLevel(), allocator);
    doc.AddMember("log-file",      m_logFile.toJSON(), allocator);
    doc.AddMember("mode",          StringRef(modeName()), allocator);
    doc.AddMember("pools",         m_pools.toJSON(doc), allocator);
    doc.AddMember("retries",       m_pools.retries(), allocator);
    doc.AddMember("retry-pause",   m_pools.retryPause(), allocator);
    doc.AddMember("reuse-timeout", reuseTimeout(), allocator);

#   ifdef XMRIG_FEATURE_TLS
    doc.AddMember("tls", m_tls.toJSON(doc), allocator);
#   endif

    doc.AddMember("user-agent",   m_userAgent.toJSON(), allocator);
    doc.AddMember("syslog",       isSyslog(), allocator);
    doc.AddMember("verbose",      isVerbose(), allocator);
    doc.AddMember("watch",        m_watch,     allocator);
    doc.AddMember("workers",      Workers::modeToJSON(workersMode()), allocator);
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
