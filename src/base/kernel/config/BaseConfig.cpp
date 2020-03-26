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


#include "base/kernel/config/BaseConfig.h"
#include "base/io/json/Json.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IJsonReader.h"
#include "rapidjson/document.h"
#include "version.h"


#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <uv.h>


#ifdef XMRIG_FEATURE_TLS
#   include <openssl/opensslv.h>
#endif

#ifdef XMRIG_FEATURE_HWLOC
#   include "backend/cpu/Cpu.h"
#endif


bool xmrig::BaseConfig::read(const IJsonReader &reader, const char *fileName)
{
    m_fileName = fileName;

    if (reader.isEmpty()) {
        return false;
    }

    m_autoSave     = reader.getBool("autosave", m_autoSave);
    m_background   = reader.getBool("background", m_background);
    m_dryRun       = reader.getBool("dry-run", m_dryRun);
    m_syslog       = reader.getBool("syslog", m_syslog);
    m_watch        = reader.getBool("watch", m_watch);
    m_logFile      = reader.getString("log-file");
    m_userAgent    = reader.getString("user-agent");
    m_version      = reader.getUint("version");

    Log::setColors(reader.getBool("colors", Log::isColors()));
    setPrintTime(reader.getUint("print-time", 60));
    setVerbose(reader.getValue("verbose"));

    const rapidjson::Value &api = reader.getObject("api");
    if (api.IsObject()) {
        m_apiId       = Json::getString(api, "id");
        m_apiWorkerId = Json::getString(api, "worker-id");
    }

    m_http.load(reader.getObject("http"));
    m_pools.load(reader);

    return m_pools.active() > 0;
}


bool xmrig::BaseConfig::save()
{
    if (m_fileName.isNull()) {
        return false;
    }

    rapidjson::Document doc;
    getJSON(doc);

    if (Json::save(m_fileName, doc)) {
        LOG_NOTICE("configuration saved to: \"%s\"", m_fileName.data());
        return true;
    }

    return false;
}


void xmrig::BaseConfig::printVersions()
{
    char buf[256] = { 0 };

#   if defined(__clang__)
    snprintf(buf, sizeof buf, "clang/%d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#   elif defined(__GNUC__)
    snprintf(buf, sizeof buf, "gcc/%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#   elif defined(_MSC_VER)
    snprintf(buf, sizeof buf, "MSVC/%d", MSVC_VERSION);
#   endif

    Log::print(GREEN_BOLD(" * ") WHITE_BOLD("%-13s") CYAN_BOLD("%s/%s") WHITE_BOLD(" %s"), "ABOUT", APP_NAME, APP_VERSION, buf);

    std::string libs;

#   if defined(XMRIG_FEATURE_TLS) && defined(OPENSSL_VERSION_TEXT)
    {
        constexpr const char *v = OPENSSL_VERSION_TEXT + 8;
        snprintf(buf, sizeof buf, "OpenSSL/%.*s ", static_cast<int>(strchr(v, ' ') - v), v);
        libs += buf;
    }
#   endif

#   if defined(XMRIG_FEATURE_HWLOC)
    libs += Cpu::info()->backend();
#   endif

    Log::print(GREEN_BOLD(" * ") WHITE_BOLD("%-13slibuv/%s %s"), "LIBS", uv_version_string(), libs.c_str());
}


void xmrig::BaseConfig::setVerbose(const rapidjson::Value &value)
{
    if (value.IsBool()) {
        Log::setVerbose(value.GetBool() ? 1 : 0);
    }
    else if (value.IsUint()) {
        Log::setVerbose(value.GetUint());
    }
}
