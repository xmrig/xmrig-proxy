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


#include "api/Api.h"
#include "common/net/SubmitResult.h"
#include "Counters.h"
#include "interfaces/ISplitter.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/Stats.h"


Stats::Stats() :
    m_hashrate(4)
{
    m_data.startTime = uv_now(uv_default_loop());
}


Stats::~Stats()
{
}


void Stats::tick(uint64_t ticks, const ISplitter *splitter)
{
    ticks++;

    if ((ticks % m_hashrate.tickTime()) == 0) {
        m_hashrate.tick();

#       ifndef XMRIG_NO_API
        m_data.hashrate[0] = hashrate(60);
        m_data.hashrate[1] = hashrate(600);
        m_data.hashrate[2] = hashrate(3600);
        m_data.hashrate[3] = hashrate(3600 * 12);
        m_data.hashrate[4] = hashrate(3600 * 24);
        m_data.hashrate[5] = hashrate(m_data.uptime());

        m_data.upstreams = splitter->upstreams();
        m_data.miners    = Counters::miners();
        m_data.maxMiners = Counters::maxMiners();
        m_data.expired   = Counters::expired;
#       endif
    }
}


void Stats::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::ConnectionType:
        m_data.connections++;
        break;

    case IEvent::CloseType:
        m_data.connections--;
        break;

    case IEvent::AcceptType:
        accept(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


void Stats::onRejectedEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::SubmitType:
        m_data.invalid++;
        break;

    case IEvent::AcceptType:
        reject(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


void Stats::accept(const AcceptEvent *event)
{
    m_hashrate.add(event->result.diff);

    m_data.accepted++;
    m_data.hashes += event->result.diff;

    if (event->isDonate()) {
        m_data.donateHashes += event->result.diff;
    }

    Counters::accepted++;

    const size_t ln = m_data.topDiff.size() - 1;
    if (event->result.actualDiff > m_data.topDiff[ln]) {
        m_data.topDiff[ln] = event->result.actualDiff;
        std::sort(m_data.topDiff.rbegin(), m_data.topDiff.rend());
    }

    m_data.latency.push_back(event->result.elapsed > 0xFFFF ? 0xFFFF : (uint16_t) event->result.elapsed);
}


void Stats::reject(const AcceptEvent *event)
{
    if (event->isDonate()) {
        return;
    }

    m_data.rejected++;
}
