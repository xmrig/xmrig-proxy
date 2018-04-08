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

#include <string.h>
#include <uv.h>


#include "core/Config.h"
#include "core/ConfigLoader.h"
#include "donate.h"
#include "log/Log.h"
#include "net/Url.h"
#include "proxy/Addr.h"
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"


static const char *algoNames[] = {
    "cryptonight",
    "cryptonight-lite"
};


static const char *modeNames[] = {
    "nicehash",
    "simple"
};


#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif


#ifdef _WIN32
#   define strncasecmp _strnicmp
#endif


xmrig::Config::Config() :
    m_adjusted(false),
    m_apiIPv6(true),
    m_apiRestricted(true),
    m_background(false),
    m_colors(true),
    m_debug(false),
    m_ready(false),
    m_syslog(false),
    m_verbose(false),
    m_watch(true),
    m_workers(true),
    m_accessLog(nullptr),
    m_apiToken(nullptr),
    m_apiWorkerId(nullptr),
    m_fileName(nullptr),
    m_logFile(nullptr),
    m_userAgent(nullptr),
    m_algorithm(CRYPTONIGHT),
    m_apiPort(0),
    m_donateLevel(kDonateLevel),
    m_mode(NICEHASH_MODE),
    m_retries(2),
    m_retryPause(1),
    m_reuseTimeout(0),
    m_diff(0)
{
    m_pools.push_back(new Url());
}


xmrig::Config::~Config()
{
    for (Addr *addr : m_addrs) {
        delete addr;
    }

    for (Url *url : m_pools) {
        delete url;
    }

    m_addrs.clear();
    m_pools.clear();

    free(m_fileName);
    free(m_accessLog);
    free(m_apiToken);
    free(m_apiWorkerId);
    free(m_logFile);
    free(m_userAgent);
}


bool xmrig::Config::isValid() const
{
    return m_pools[0]->isValid();
}


bool xmrig::Config::reload(const char *json)
{
    return xmrig::ConfigLoader::reload(this, json);
}


bool xmrig::Config::save()
{
    if (!m_fileName) {
        return false;
    }

    uv_fs_t req;
    const int fd = uv_fs_open(uv_default_loop(), &req, m_fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644, nullptr);
    if (fd < 0) {
        return false;
    }

    uv_fs_req_cleanup(&req);

    rapidjson::Document doc;
    getJSON(doc);

    FILE *fp = fdopen(fd, "w");

    char buf[4096];
    rapidjson::FileWriteStream os(fp, buf, sizeof(buf));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);

    fclose(fp);

    uv_fs_close(uv_default_loop(), &req, fd, nullptr);
    uv_fs_req_cleanup(&req);

    LOG_NOTICE("configuration saved to: \"%s\"", m_fileName);
    return true;
}


const char *xmrig::Config::algoName() const
{
    return algoNames[m_algorithm];
}


const char *xmrig::Config::modeName() const
{
    return modeNames[m_mode];
}


void xmrig::Config::getJSON(rapidjson::Document &doc)
{
    doc.SetObject();

    auto &allocator = doc.GetAllocator();

    doc.AddMember("access-log-file", accessLog() ? rapidjson::Value(rapidjson::StringRef(accessLog())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
    doc.AddMember("algo",            rapidjson::StringRef(algoName()), allocator);

    rapidjson::Value api(rapidjson::kObjectType);
    api.AddMember("port",         apiPort(), allocator);
    api.AddMember("access-token", apiToken() ? rapidjson::Value(rapidjson::StringRef(apiToken())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
    api.AddMember("worker-id",    apiWorkerId() ? rapidjson::Value(rapidjson::StringRef(apiWorkerId())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
    api.AddMember("ipv6",         apiIPv6(), allocator);
    api.AddMember("restricted",   apiRestricted(), allocator);
    doc.AddMember("api",          api, allocator);

    doc.AddMember("background",   background(), allocator);

    rapidjson::Value bind(rapidjson::kArrayType);
    for (const Addr *addr : m_addrs) {
        bind.PushBack(rapidjson::StringRef(addr->addr()), allocator);
    }

    doc.AddMember("bind",         bind, allocator);
    doc.AddMember("colors",       colors(), allocator);
    doc.AddMember("custom-diff",  diff(), allocator);
    doc.AddMember("donate-level", donateLevel(), allocator);
    doc.AddMember("log-file",     logFile() ? rapidjson::Value(rapidjson::StringRef(logFile())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
    doc.AddMember("mode",         rapidjson::StringRef(modeName()), allocator);

    rapidjson::Value pools(rapidjson::kArrayType);

    for (const Url *url : m_pools) {
        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("url",     rapidjson::StringRef(url->url()), allocator);
        obj.AddMember("user",    rapidjson::StringRef(url->user()), allocator);
        obj.AddMember("pass",    rapidjson::StringRef(url->password()), allocator);
        obj.AddMember("coin",    rapidjson::StringRef(url->coin()), allocator);
        obj.AddMember("variant", url->variant(), allocator);

        pools.PushBack(obj, allocator);
    }

    doc.AddMember("pools", pools, allocator);

    doc.AddMember("retries",       retries(), allocator);
    doc.AddMember("retry-pause",   retryPause(), allocator);
    doc.AddMember("reuse-timeout", reuseTimeout(), allocator);
    doc.AddMember("user-agent",    userAgent() ? rapidjson::Value(rapidjson::StringRef(userAgent())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);

#   ifdef HAVE_SYSLOG_H
    doc.AddMember("syslog", syslog(), allocator);
#   endif

    doc.AddMember("verbose",      verbose(), allocator);
    doc.AddMember("watch",        m_watch,   allocator);
    doc.AddMember("workers",      workers(), allocator);
}


xmrig::Config *xmrig::Config::load(int argc, char **argv, IWatcherListener *listener)
{
    return xmrig::ConfigLoader::load(argc, argv, listener);
}


void xmrig::Config::adjust()
{
    if (m_adjusted) {
        return;
    }

    m_adjusted = true;

    for (Url *url : m_pools) {
        url->adjust(algorithm());
    }
}


void xmrig::Config::setAlgo(const char *algo)
{
    const size_t size = sizeof(algoNames) / sizeof((algoNames)[0]);

    for (size_t i = 0; i < size; i++) {
        if (algoNames[i] && !strcmp(algo, algoNames[i])) {
            m_algorithm = (int) i;
            break;
        }

        if (i == size - 1 && !strcmp(algo, "cryptonight-light")) {
            m_algorithm = CRYPTONIGHT_LITE;
            break;
        }
    }
}


void xmrig::Config::setCoin(const char *coin)
{
    if (strncasecmp(coin, "aeon", 4) == 0) {
        m_algorithm = CRYPTONIGHT_LITE;
    }
}


void xmrig::Config::setFileName(const char *fileName)
{
    free(m_fileName);
    m_fileName = fileName ? strdup(fileName) : nullptr;
}


void xmrig::Config::setMode(const char *mode)
{
    const size_t size = sizeof(modeNames) / sizeof((modeNames)[0]);

    for (size_t i = 0; i < size; i++) {
        if (modeNames[i] && !strcmp(mode, modeNames[i])) {
            m_mode = (int) i;
            break;
        }
    }
}
