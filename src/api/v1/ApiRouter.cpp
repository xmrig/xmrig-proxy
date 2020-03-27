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

#include <cmath>
#include <string.h>
#include <uv.h>
#include <thread>


#include "api/v1/ApiRouter.h"
#include "base/tools/Buffer.h"
#include "common/crypto/keccak.h"
#include "common/Platform.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/Miner.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "version.h"
#include "api/interfaces/IApiRequest.h"


static inline double normalize(double d)
{
    if (!std::isnormal(d)) {
        return 0.0;
    }

    return std::floor(d * 100.0) / 100.0;
}


xmrig::ApiRouter::ApiRouter(Controller *controller) :
    m_controller(controller)
{
}


xmrig::ApiRouter::~ApiRouter()
{
}


void xmrig::ApiRouter::onRequest(IApiRequest &request)
{
    if (request.method() == IApiRequest::METHOD_GET) {
        if (request.url() == "/1/summary") {
            request.accept();
            getMiner(request.reply(), request.doc());
            getHashrate(request.reply(), request.doc());
            getResourcesSummary(request.reply(), request.doc());
            getMinersSummary(request.reply(), request.doc());
            getResults(request.reply(), request.doc());
        }
        else if (request.url() == "/1/workers") {
            request.accept();
            getHashrate(request.reply(), request.doc());
            getWorkers(request.reply(), request.doc());
        }
        else if (request.url() == "/1/miners") {
            request.accept();
            getMiners(request.reply(), request.doc());
        }
        else if (request.url() == "/1/config") {
            if (request.isRestricted()) {
                return request.done(403);
            }

            m_controller->config()->getJSON(request.doc());
        }
    }
    else if (request.method() == IApiRequest::METHOD_PUT || request.method() == IApiRequest::METHOD_POST) {
        if (request.url() == "/1/config") {
            request.accept();

            if (!m_controller->config()->reload(request.json())) {
                return request.done(400);
            }

            request.done(204);
        }
    }
}


void xmrig::ApiRouter::getHashrate(rapidjson::Value &reply, rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();

    rapidjson::Value hashrate(rapidjson::kObjectType);
    rapidjson::Value total(rapidjson::kArrayType);

    auto &stats = m_controller->statsData();

    for (size_t i = 0; i < sizeof(stats.hashrate) / sizeof(stats.hashrate[0]); i++) {
        total.PushBack(normalize(stats.hashrate[i]), allocator);
    }

    hashrate.AddMember("total", total, allocator);
    reply.AddMember("hashrate", hashrate, allocator);
}


void xmrig::ApiRouter::getMiner(rapidjson::Value &reply, rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    auto &stats = m_controller->statsData();

    reply.AddMember("version",      APP_VERSION, allocator);
    reply.AddMember("kind",         APP_KIND, allocator);
    reply.AddMember("algo",         rapidjson::StringRef(m_controller->config()->algorithm().name()), allocator);
    reply.AddMember("mode",         rapidjson::StringRef(m_controller->config()->modeName()), allocator);
    reply.AddMember("ua",           rapidjson::StringRef(Platform::userAgent()), allocator);
    reply.AddMember("uptime",       stats.uptime(), allocator);
    reply.AddMember("donate_level", m_controller->config()->pools().donateLevel(), allocator);

    if (stats.hashes && stats.donateHashes) {
        reply.AddMember("donated", normalize((double) stats.donateHashes / stats.hashes * 100.0), allocator);
    }
    else {
        reply.AddMember("donated", 0.0, allocator);
    }
}


void xmrig::ApiRouter::getMiners(rapidjson::Value &reply, rapidjson::Document &doc) const
{
    using namespace rapidjson;

    auto &allocator = doc.GetAllocator();
    auto list       = m_controller->miners();

    Value miners(kArrayType);

    for (const xmrig::Miner *miner : list) {
        if (miner->mapperId() == -1) {
            continue;
        }

        Value value(kArrayType);
        value.PushBack(miner->id(),                allocator);
        value.PushBack(StringRef(miner->ip()),     allocator);
        value.PushBack(miner->tx(),                allocator);
        value.PushBack(miner->rx(),                allocator);
        value.PushBack(miner->state(),             allocator);
        value.PushBack(miner->diff(),              allocator);
        value.PushBack(miner->user().toJSON(),     allocator);
        value.PushBack(miner->password().toJSON(), allocator);
        value.PushBack(miner->rigId().toJSON(),    allocator);
        value.PushBack(miner->agent().toJSON(),    allocator);

        miners.PushBack(value, allocator);
    }

    Value format(kArrayType);
    format.PushBack("id",       allocator);
    format.PushBack("ip",       allocator);
    format.PushBack("tx",       allocator);
    format.PushBack("rx",       allocator);
    format.PushBack("state",    allocator);
    format.PushBack("diff",     allocator);
    format.PushBack("user",     allocator);
    format.PushBack("password", allocator);
    format.PushBack("rig_id",   allocator);
    format.PushBack("agent",    allocator);

    reply.AddMember("format", format, allocator);
    reply.AddMember("miners", miners, allocator);
}


void xmrig::ApiRouter::getMinersSummary(rapidjson::Value &reply, rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    auto &stats = m_controller->statsData();

    rapidjson::Value miners(rapidjson::kObjectType);

    miners.AddMember("now", stats.miners, allocator);
    miners.AddMember("max", stats.maxMiners, allocator);

    reply.AddMember("miners",  miners, allocator);
    reply.AddMember("workers", static_cast<uint64_t>(m_controller->workers().size()), allocator);

    rapidjson::Value upstreams(rapidjson::kObjectType);

    upstreams.AddMember("active", stats.upstreams.active, allocator);
    upstreams.AddMember("sleep",  stats.upstreams.sleep, allocator);
    upstreams.AddMember("error",  stats.upstreams.error, allocator);
    upstreams.AddMember("total",  stats.upstreams.total, allocator);
    upstreams.AddMember("ratio",  normalize(stats.upstreams.ratio), allocator);

    reply.AddMember("upstreams", upstreams, allocator);
}



void xmrig::ApiRouter::getResourcesSummary(rapidjson::Value &reply, rapidjson::Document &doc) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    size_t rss = 0;
    uv_resident_set_memory(&rss);

    Value resources(kObjectType);
    Value memory(kObjectType);
    Value load_average(kArrayType);

    memory.AddMember("total",               uv_get_total_memory(), allocator);
    memory.AddMember("resident_set_memory", static_cast<uint64_t>(rss), allocator);

    double loadavg[3] = { 1.0 };
    uv_loadavg(loadavg);
    load_average.PushBack(loadavg[0], allocator);
    load_average.PushBack(loadavg[1], allocator);
    load_average.PushBack(loadavg[2], allocator);

    resources.AddMember("memory", memory, allocator);
    resources.AddMember("load_average", load_average, allocator);
    resources.AddMember("hardware_concurrency", std::thread::hardware_concurrency(), allocator);

    reply.AddMember("resources", resources, allocator);
}


void xmrig::ApiRouter::getResults(rapidjson::Value &reply, rapidjson::Document &doc) const
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

    reply.AddMember("results", results, allocator);
}


void xmrig::ApiRouter::getWorkers(rapidjson::Value &reply, rapidjson::Document &doc) const
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

    reply.AddMember("mode", StringRef(Workers::modeName(m_controller->config()->workersMode())), allocator);
    reply.AddMember("workers", workers, allocator);
}
