/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
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

#include <cmath>
#include <string.h>
#include <uv.h>

#if _WIN32
#   include "winsock2.h"
#else
#   include "unistd.h"
#endif


#include "api/ApiState.h"
#include "net/Job.h"
#include "Options.h"
#include "Platform.h"
#include "version.h"


extern "C"
{
#include "crypto/c_keccak.h"
}


static inline double normalize(double d)
{
    if (!std::isnormal(d)) {
        return 0.0;
    }

    return std::floor(d * 10.0) / 10.0;
}


ApiState::ApiState()
{
    memset(m_workerId, 0, sizeof(m_workerId));

    if (Options::i()->apiWorkerId()) {
        strncpy(m_workerId, Options::i()->apiWorkerId(), sizeof(m_workerId) - 1);
    }
    else {
        gethostname(m_workerId, sizeof(m_workerId) - 1);
    }

    genId();
}


ApiState::~ApiState()
{
}


char *ApiState::get(const char *url, int *status) const
{
    json_t *reply = json_object();

    if (strncmp(url, "/workers.json", 13) == 0) {
        getHashrate(reply);
        getWorkers(reply);

        return finalize(reply);
    }

    getIdentify(reply);
    getMiner(reply);
    getHashrate(reply);
    getMinersSummary(reply);
    getResults(reply);

    return finalize(reply);
}


void ApiState::tick(const StatsData &data)
{
    m_stats = data;
}


void ApiState::tick(const std::vector<Worker> &workers)
{
    m_workers = workers;
}


char *ApiState::finalize(json_t *reply) const
{
    char *buf = json_dumps(reply, JSON_INDENT(4) | JSON_REAL_PRECISION(15));
    json_decref(reply);

    return buf;
}


void ApiState::genId()
{
    memset(m_id, 0, sizeof(m_id));

    uv_interface_address_t *interfaces;
    int count = 0;

    if (uv_interface_addresses(&interfaces, &count) < 0) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (!interfaces[i].is_internal && interfaces[i].address.address4.sin_family == AF_INET) {
            uint8_t hash[200];
            const size_t addrSize = sizeof(interfaces[i].phys_addr);
            const size_t inSize   = strlen(APP_KIND) + addrSize;

            uint8_t *input = new uint8_t[inSize]();
            memcpy(input, interfaces[i].phys_addr, addrSize);
            memcpy(input + addrSize, APP_KIND, strlen(APP_KIND));

            keccak(input, static_cast<int>(inSize), hash, sizeof(hash));
            Job::toHex(hash, 8, m_id);
            break;
        }
    }

    uv_free_interface_addresses(interfaces, count);
}


void ApiState::getHashrate(json_t *reply) const
{
    json_t *hashrate = json_object();
    json_t *total    = json_array();

    json_object_set(reply,    "hashrate", hashrate);
    json_object_set(hashrate, "total",    total);

    for (size_t i = 0; i < sizeof(m_stats.hashrate) / sizeof(m_stats.hashrate[0]); i++) {
        json_array_append(total, json_real(normalize(m_stats.hashrate[i])));
    }
}


void ApiState::getIdentify(json_t *reply) const
{
    json_object_set(reply, "id",        json_string(m_id));
    json_object_set(reply, "worker_id", json_string(m_workerId));
}


void ApiState::getMiner(json_t *reply) const
{
    json_object_set(reply, "version",   json_string(APP_VERSION));
    json_object_set(reply, "kind",      json_string(APP_KIND));
    json_object_set(reply, "ua",        json_string(Platform::userAgent()));
    json_object_set(reply, "donate",    json_integer(Options::i()->donateLevel()));
    json_object_set(reply, "uptime",    json_integer(m_stats.uptime()));
}


void ApiState::getMinersSummary(json_t *reply) const
{
    json_t *miners = json_object();
    json_object_set(reply, "miners", miners);

    json_object_set(miners, "now",      json_integer(m_stats.miners));
    json_object_set(miners, "max",      json_integer(m_stats.maxMiners));
    json_object_set(reply, "upstreams", json_integer(m_stats.upstreams));
}


void ApiState::getResults(json_t *reply) const
{
    json_t *results = json_object();
    json_t *best    = json_array();

    json_object_set(reply,   "results",       results);
    json_object_set(results, "accepted",      json_integer(m_stats.accepted));
    json_object_set(results, "rejected",      json_integer(m_stats.rejected));
    json_object_set(results, "invalid",       json_integer(m_stats.invalid));
    json_object_set(results, "avg_time",      json_integer(m_stats.avgTime()));
    json_object_set(results, "latency",       json_integer(m_stats.avgLatency()));
    json_object_set(results, "hashes_total",  json_integer(m_stats.hashes));
    json_object_set(results, "best",          best);
    json_object_set(results, "error_log",     json_array());

    for (size_t i = 0; i < m_stats.topDiff.size(); ++i) {
        json_array_append(best, json_integer(m_stats.topDiff[i]));
    }
}


void ApiState::getWorkers(json_t *reply) const
{
    json_t *workers = json_array();

    json_object_set(reply, "workers", workers);

    for (const Worker &worker : m_workers) {
        if (worker.connections() == 0 && worker.lastHash() == 0) {
            continue;
        }

        json_t *array = json_array();
        json_array_append(array, json_string(worker.name()));
        json_array_append(array, json_string(worker.ip()));
        json_array_append(array, json_integer(worker.connections()));
        json_array_append(array, json_integer(worker.accepted()));
        json_array_append(array, json_integer(worker.rejected()));
        json_array_append(array, json_integer(worker.invalid()));
        json_array_append(array, json_integer(worker.hashes()));
        json_array_append(array, json_integer(worker.lastHash()));
        json_array_append(array, json_real(normalize(worker.hashrate(60))));
        json_array_append(array, json_real(normalize(worker.hashrate(600))));
        json_array_append(array, json_real(normalize(worker.hashrate(3600))));
        json_array_append(array, json_real(normalize(worker.hashrate(3600 * 12))));
        json_array_append(array, json_real(normalize(worker.hashrate(3600 * 24))));

        json_array_append(workers, array);
    }
}
