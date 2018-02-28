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

#include <string.h>


#include "core/Config.h"
#include "core/ConfigLoader.h"
#include "donate.h"
#include "net/Url.h"
#include "proxy/Addr.h"
#include "rapidjson/document.h"


static const char *algo_names[] = {
    "cryptonight",
    "cryptonight-lite"
};


#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif


#ifdef _WIN32
#   define strncasecmp _strnicmp
#endif


xmrig::Config::Config() :
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
    m_retries(5),
    m_retryPause(5),
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


const char *xmrig::Config::algoName() const
{
    return algo_names[m_algorithm];
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
    doc.AddMember("api",          api, allocator);

    doc.AddMember("background",   background(), allocator);
    doc.AddMember("colors",       colors(), allocator);
    doc.AddMember("custom-diff",  diff(), allocator);
    doc.AddMember("donate-level", donateLevel(), allocator);
    doc.AddMember("log-file",     logFile() ? rapidjson::Value(rapidjson::StringRef(logFile())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);

    rapidjson::Value pools(rapidjson::kArrayType);

    for (const Url *url : m_pools) {
        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("url",       rapidjson::StringRef(url->url()), allocator);
        obj.AddMember("user",      rapidjson::StringRef(url->user()), allocator);
        obj.AddMember("pass",      rapidjson::StringRef(url->password()), allocator);

        pools.PushBack(obj, allocator);
    }

    doc.AddMember("pools", pools, allocator);

    doc.AddMember("retries",      retries(), allocator);
    doc.AddMember("retry-pause",  retryPause(), allocator);
    doc.AddMember("user-agent",   userAgent() ? rapidjson::Value(rapidjson::StringRef(userAgent())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);

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


void xmrig::Config::setAlgo(const char *algo)
{
    const size_t size = sizeof(algo_names) / sizeof((algo_names)[0]);

    for (size_t i = 0; i < size; i++) {
        if (algo_names[i] && !strcmp(algo, algo_names[i])) {
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
    m_fileName = strdup(fileName);
}
