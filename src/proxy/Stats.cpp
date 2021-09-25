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

#include "base/net/stratum/SubmitResult.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/events/ConfigEvent.h"
#include "base/kernel/events/SaveEvent.h"
#include "base/kernel/Process.h"
#include "base/tools/Arguments.h"
#include "core/Controller.h"
#include "Counters.h"
#include "interfaces/ISplitter.h"
#include "proxy/config/MainConfig.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/Stats.h"
#include "proxy/StatsData.h"


namespace xmrig {


class Stats::Private
{
public:
    constexpr static const char *kKey   = "custom-diff-stats";

    inline double calc(int seconds) const       { return hashrate.calc(seconds); }
    inline void read(const ConfigEvent *event)  { customDiffStats = event->reader()->getBool(kKey, customDiffStats); }

    inline void save(rapidjson::Document &doc) const
    {
        using namespace rapidjson;

        doc.AddMember(StringRef(kKey), customDiffStats ? Value(customDiffStats) : Value(kNullType), doc.GetAllocator());
    }

    void accept(AcceptEvent *event);
    void printHashrate() const;
    void reject(const AcceptEvent *event);
    void tick(uint64_t ticks, const ISplitter *splitter);

    bool customDiffStats    = false;
    StatsData data;
    TickingCounter<uint32_t> hashrate{4};
};


} // namespace xmrig


xmrig::Stats::Stats(const ConfigEvent *event) :
    d(std::make_shared<Private>())
{
    d->customDiffStats = Process::arguments().contains("--custom-diff-stats");
    d->read(event);
}


const xmrig::StatsData &xmrig::Stats::data() const
{
    return d->data;
}


double xmrig::Stats::hashrate(int seconds) const
{
    return d->calc(seconds);
}


void xmrig::Stats::tick(uint64_t ticks, const ISplitter *splitter)
{
    d->tick(ticks, splitter);
}


void xmrig::Stats::onEvent(uint32_t type, IEvent *event)
{
    if (!event->isRejected()) {
        switch (type) {
        case CONNECTION_EVENT:
            d->data.connections++;
            break;

        case CLOSE_EVENT:
            d->data.connections--;
            break;

        case ACCEPT_EVENT:
            d->accept(static_cast<AcceptEvent *>(event));
            break;

        case IEvent::CONSOLE:
            if (event->data() == 'h' || event->data() == 'H') {
                d->printHashrate();
            }
            break;

        case IEvent::CONFIG:
            if (event->data() == 0) {
                d->read(static_cast<const ConfigEvent *>(event));
            }
            break;

        case IEvent::SAVE:
            if (event->data() == 0) {
                d->save(static_cast<SaveEvent *>(event)->doc());
            }
            break;

        default:
            break;
        }
    }
    else {
        switch (type) {
        case SUBMIT_EVENT:
            d->data.invalid++;
            break;

        case ACCEPT_EVENT:
            d->reject(static_cast<const AcceptEvent*>(event));
            break;

        default:
            break;
        }
    }
}


void xmrig::Stats::Private::accept(AcceptEvent *event)
{
    event->setCustomDiffStats(customDiffStats);

    if (event->isCustomDiff() && !customDiffStats) {
        return;
    }

    hashrate.add(event->statsDiff());

    if (event->isCustomDiff()) {
        return;
    }

    data.accepted++;
    data.hashes += event->result.diff;

    if (event->isDonate()) {
        data.donateHashes += event->result.diff;
    }

    Counters::accepted++;

    const size_t ln = data.topDiff.size() - 1;
    if (event->result.actualDiff > data.topDiff[ln]) {
        data.topDiff[ln] = event->result.actualDiff;
        std::sort(data.topDiff.rbegin(), data.topDiff.rend());
    }

    data.latency.push_back(event->result.elapsed > 0xFFFF ? 0xFFFF : (uint16_t) event->result.elapsed);
}


void xmrig::Stats::Private::printHashrate() const
{
    LOG_INFO("%s " WHITE_BOLD("speed") " (1m) " CYAN_BOLD("%03.2f") " (10m) " CYAN_BOLD("%03.2f") " (1h) " CYAN_BOLD("%03.2f") " (12h) " CYAN_BOLD("%03.2f") " (24h) " CYAN_BOLD("%03.2f kH/s"),
             Tags::proxy(), calc(60), calc(600), calc(3600), calc(3600 * 12), calc(3600 * 24));
}


void xmrig::Stats::Private::reject(const AcceptEvent *event)
{
    if (event->isDonate()) {
        return;
    }

    data.rejected++;
}


void xmrig::Stats::Private::tick(uint64_t ticks, const ISplitter *splitter)
{
    ticks++;

    if ((ticks % hashrate.tickTime()) == 0) {
        hashrate.tick();

#       ifdef XMRIG_FEATURE_API
        data.hashrate[0]    = calc(60);
        data.hashrate[1]    = calc(600);
        data.hashrate[2]    = calc(3600);
        data.hashrate[3]    = calc(3600 * 12);
        data.hashrate[4]    = calc(3600 * 24);
        data.hashrate[5]    = calc(static_cast<int>(data.uptime()));

        data.upstreams      = splitter->upstreams();
        data.miners         = Counters::miners();
        data.maxMiners      = Counters::maxMiners();
        data.expired        = Counters::expired;
#       endif
    }
}
