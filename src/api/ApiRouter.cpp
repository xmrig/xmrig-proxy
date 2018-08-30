/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "api/ApiRouter.h"
#include "common/api/HttpReply.h"
#include "common/api/HttpRequest.h"
#include "common/crypto/keccak.h"
#include "common/net/Job.h"
#include "common/Platform.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "proxy/Miner.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "version.h"


static inline double normalize(double d)
{
    if (!std::isnormal(d)) {
        return 0.0;
    }

    return std::floor(d * 100.0) / 100.0;
}


ApiRouter::ApiRouter(xmrig::Controller *controller) :
    m_controller(controller)
{
    setWorkerId(controller->config()->apiWorkerId());
    genId(controller->config()->apiId());

    controller->addListener(this);
}


ApiRouter::~ApiRouter()
{
}


void ApiRouter::get(const xmrig::HttpRequest &req, xmrig::HttpReply &reply) const
{
    rapidjson::Document doc;
    doc.SetObject();

    if (req.match("/1/workers") || req.match("/workers.json")) {
        getHashrate(doc);
        getWorkers(doc);

        return finalize(reply, doc);
    }

    if (req.match("/1/miners")) {
        getMiners(doc);

        return finalize(reply, doc);
    }

    if (req.match("/1/config")) {
        if (req.isRestricted()) {
            reply.status = 403;
            return;
        }

        m_controller->config()->getJSON(doc);

        return finalize(reply, doc);
    }

    if (req.match("/1/resources") || req.match("/resources.json")) {
        getResources(doc);

        return finalize(reply, doc);
    }

    getIdentify(doc);
    getMiner(doc);
    getHashrate(doc);
    getMinersSummary(doc, req.match("/1/summary"));
    getResults(doc);

    return finalize(reply, doc);
}

void ApiRouter::exec(const xmrig::HttpRequest &req, xmrig::HttpReply &reply)
{
    if (req.method() == xmrig::HttpRequest::Put && req.match("/1/config")) {
        m_controller->config()->reload(req.body());
        return;
    }

    reply.status = 404;
}


void ApiRouter::onConfigChanged(xmrig::Config *config, xmrig::Config *previousConfig)
{
    updateWorkerId(config->apiWorkerId(), previousConfig->apiWorkerId());
}


void ApiRouter::finalize(xmrig::HttpReply &reply, rapidjson::Document &doc) const
{
    rapidjson::StringBuffer buffer(0, 4096);
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetMaxDecimalPlaces(10);
    doc.Accept(writer);

    reply.status = 200;
    reply.buf    = strdup(buffer.GetString());
    reply.size   = buffer.GetSize();
}


void ApiRouter::genId(const char *id)
{
    memset(m_id, 0, sizeof(m_id));

    if (id && strlen(id) > 0) {
        strncpy(m_id, id, sizeof(m_id) - 1);
        return;
    }

    uv_interface_address_t *interfaces;
    int count = 0;

    if (uv_interface_addresses(&interfaces, &count) < 0) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (!interfaces[i].is_internal && interfaces[i].address.address4.sin_family == AF_INET) {
            uint8_t hash[200];
            const size_t addrSize = sizeof(interfaces[i].phys_addr);
            const size_t inSize   = strlen(APP_KIND) + addrSize + sizeof(uint16_t);
            const uint16_t port   = static_cast<uint16_t>(m_controller->config()->apiPort());

            uint8_t *input = new uint8_t[inSize]();
            memcpy(input, &port, sizeof(uint16_t));
            memcpy(input + sizeof(uint16_t), interfaces[i].phys_addr, addrSize);
            memcpy(input + sizeof(uint16_t) + addrSize, APP_KIND, strlen(APP_KIND));

            xmrig::keccak(input, inSize, hash);
            Job::toHex(hash, 8, m_id);

            delete [] input;
            break;
        }
    }

    uv_free_interface_addresses(interfaces, count);
}


void ApiRouter::getHashrate(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();

    rapidjson::Value hashrate(rapidjson::kObjectType);
    rapidjson::Value total(rapidjson::kArrayType);

    auto &stats = m_controller->statsData();

    for (size_t i = 0; i < sizeof(stats.hashrate) / sizeof(stats.hashrate[0]); i++) {
        total.PushBack(normalize(stats.hashrate[i]), allocator);
    }

    hashrate.AddMember("total", total, allocator);
    doc.AddMember("hashrate", hashrate, allocator);
}


void ApiRouter::getIdentify(rapidjson::Document &doc) const
{
    doc.AddMember("id",        rapidjson::StringRef(m_id),       doc.GetAllocator());
    doc.AddMember("worker_id", rapidjson::StringRef(m_workerId), doc.GetAllocator());
}


void ApiRouter::getMiner(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    auto &stats = m_controller->statsData();

    doc.AddMember("version",      APP_VERSION, allocator);
    doc.AddMember("kind",         APP_KIND, allocator);
    doc.AddMember("algo",         rapidjson::StringRef(m_controller->config()->algorithm().name()), allocator);
    doc.AddMember("mode",         rapidjson::StringRef(m_controller->config()->modeName()), allocator);
    doc.AddMember("ua",           rapidjson::StringRef(Platform::userAgent()), allocator);
    doc.AddMember("uptime",       stats.uptime(), allocator);
    doc.AddMember("donate_level", m_controller->config()->donateLevel(), allocator);

    if (stats.hashes && stats.donateHashes) {
        doc.AddMember("donated", normalize((double) stats.donateHashes / stats.hashes * 100.0), allocator);
    }
    else {
        doc.AddMember("donated", 0.0, allocator);
    }
}


void ApiRouter::getMiners(rapidjson::Document &doc) const
{
    using namespace rapidjson;

    auto &allocator = doc.GetAllocator();
    auto list       = m_controller->miners();

    Value miners(kArrayType);

    for (const Miner *miner : list) {
        if (miner->mapperId() == -1) {
            continue;
        }

        Value value(kArrayType);
        value.PushBack(miner->id(), allocator);
        value.PushBack(StringRef(miner->ip()), allocator);
        value.PushBack(miner->tx(), allocator);
        value.PushBack(miner->rx(), allocator);
        value.PushBack(miner->state(), allocator);
        value.PushBack(miner->diff(), allocator);
        value.PushBack(miner->user()     ? Value(StringRef(miner->user()))     : Value(kNullType), allocator);
        value.PushBack(miner->password() ? Value(StringRef(miner->password())) : Value(kNullType), allocator);
        value.PushBack(miner->rigId()    ? Value(StringRef(miner->rigId()))    : Value(kNullType), allocator);
        value.PushBack(miner->agent()    ? Value(StringRef(miner->agent()))    : Value(kNullType), allocator);

        miners.PushBack(value, allocator);
    }

    Value format(kArrayType);
    format.PushBack("id", allocator);
    format.PushBack("ip", allocator);
    format.PushBack("tx", allocator);
    format.PushBack("rx", allocator);
    format.PushBack("state", allocator);
    format.PushBack("diff", allocator);
    format.PushBack("user", allocator);
    format.PushBack("password", allocator);
    format.PushBack("rig_id", allocator);
    format.PushBack("agent", allocator);

    doc.AddMember("format", format, allocator);
    doc.AddMember("miners", miners, allocator);
}


void ApiRouter::getMinersSummary(rapidjson::Document &doc, bool advanced) const
{
    auto &allocator = doc.GetAllocator();
    auto &stats = m_controller->statsData();

    rapidjson::Value miners(rapidjson::kObjectType);

    miners.AddMember("now", stats.miners, allocator);
    miners.AddMember("max", stats.maxMiners, allocator);

    doc.AddMember("miners",  miners, allocator);
    doc.AddMember("workers", static_cast<uint64_t>(m_controller->workers().size()), allocator);

    if (advanced) {
        rapidjson::Value upstreams(rapidjson::kObjectType);

        upstreams.AddMember("active", stats.upstreams.active, allocator);
        upstreams.AddMember("sleep",  stats.upstreams.sleep, allocator);
        upstreams.AddMember("error",  stats.upstreams.error, allocator);
        upstreams.AddMember("total",  stats.upstreams.total, allocator);
        upstreams.AddMember("ratio",  normalize(stats.upstreams.ratio), allocator);

        doc.AddMember("upstreams", upstreams, allocator);
    }
    else {
        doc.AddMember("upstreams", stats.upstreams.active, allocator);
    }
}



void ApiRouter::getResources(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    size_t rss = 0;
    uv_resident_set_memory(&rss);

    doc.AddMember("total_memory",        uv_get_total_memory(), allocator);
    doc.AddMember("resident_set_memory", (uint64_t) rss, allocator);
}


void ApiRouter::getResults(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    auto &stats = m_controller->statsData();

    rapidjson::Value results(rapidjson::kObjectType);

    results.AddMember("accepted",      stats.accepted, allocator);
    results.AddMember("rejected",      stats.rejected, allocator);
    results.AddMember("invalid",       stats.invalid, allocator);
    results.AddMember("expired",       stats.expired, allocator);
    results.AddMember("avg_time",      stats.avgTime(), allocator);
    results.AddMember("latency",       stats.avgLatency(), allocator);
    results.AddMember("hashes_total",  stats.hashes, allocator);
    results.AddMember("hashes_donate", stats.donateHashes, allocator);

    rapidjson::Value best(rapidjson::kArrayType);
    for (size_t i = 0; i < stats.topDiff.size(); ++i) {
        best.PushBack(stats.topDiff[i], allocator);
    }

    results.AddMember("best", best, allocator);

    doc.AddMember("results", results, allocator);
}


void ApiRouter::getWorkers(rapidjson::Document &doc) const
{
    using namespace rapidjson;

    auto &allocator = doc.GetAllocator();
    auto &list      = m_controller->workers();

    Value workers(kArrayType);

    for (const Worker &worker : list) {
         Value array(kArrayType);
         array.PushBack(StringRef(worker.name()), allocator);
         array.PushBack(StringRef(worker.ip()), allocator);
         array.PushBack(worker.connections(), allocator);
         array.PushBack(worker.accepted(), allocator);
         array.PushBack(worker.rejected(), allocator);
         array.PushBack(worker.invalid(), allocator);
         array.PushBack(worker.hashes(), allocator);
         array.PushBack(worker.lastHash(), allocator);
         array.PushBack(normalize(worker.hashrate(60)), allocator);
         array.PushBack(normalize(worker.hashrate(600)), allocator);
         array.PushBack(normalize(worker.hashrate(3600)), allocator);
         array.PushBack(normalize(worker.hashrate(3600 * 12)), allocator);
         array.PushBack(normalize(worker.hashrate(3600 * 24)), allocator);

         workers.PushBack(array, allocator);
    }

    doc.AddMember("mode", StringRef(Workers::modeName(m_controller->config()->workersMode())), allocator);
    doc.AddMember("workers", workers, allocator);
}


void ApiRouter::setWorkerId(const char *id)
{
    memset(m_workerId, 0, sizeof(m_workerId));

    if (id && strlen(id) > 0) {
        strncpy(m_workerId, id, sizeof(m_workerId) - 1);
    }
    else {
        gethostname(m_workerId, sizeof(m_workerId) - 1);
    }
}


void ApiRouter::updateWorkerId(const char *id, const char *previousId)
{
    if (id == previousId) {
        return;
    }

    if (id != nullptr && previousId != nullptr && strcmp(id, previousId) == 0) {
        return;
    }

    setWorkerId(id);
}
