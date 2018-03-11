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

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#include <inttypes.h>
#include <memory>
#include <time.h>


#include "Counters.h"
#include "log/AccessLog.h"
#include "log/Log.h"
#include "log/ShareLog.h"
#include "Options.h"
#include "Platform.h"
#include "proxy/Events.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Miner.h"
#include "proxy/Miners.h"
#include "proxy/Proxy.h"
#include "proxy/ProxyDebug.h"
#include "proxy/Server.h"
#include "proxy/splitters/NonceSplitter.h"
#include "proxy/Stats.h"
#include "proxy/workers/Workers.h"


Proxy::Proxy(const Options *options) :
    m_ticks(0)
{
    srand(time(0) ^ (uintptr_t) this);

    m_miners    = new Miners();
    m_splitter  = new NonceSplitter();
    m_shareLog  = new ShareLog(m_stats);
    m_accessLog = new AccessLog();
    m_workers   = new Workers();

    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    Events::subscribe(IEvent::ConnectionType, m_miners);
    Events::subscribe(IEvent::ConnectionType, &m_stats);

    Events::subscribe(IEvent::CloseType, m_miners);
    Events::subscribe(IEvent::CloseType, m_splitter);
    Events::subscribe(IEvent::CloseType, &m_stats);
    Events::subscribe(IEvent::CloseType, m_accessLog);
    Events::subscribe(IEvent::CloseType, m_workers);

    Events::subscribe(IEvent::LoginType, &m_customDiff);
    Events::subscribe(IEvent::LoginType, m_splitter);
    Events::subscribe(IEvent::LoginType, &m_stats);
    Events::subscribe(IEvent::LoginType, m_accessLog);
    Events::subscribe(IEvent::LoginType, m_workers);

    Events::subscribe(IEvent::SubmitType, m_splitter);
    Events::subscribe(IEvent::SubmitType, &m_stats);
    Events::subscribe(IEvent::SubmitType, m_workers);

    Events::subscribe(IEvent::AcceptType, &m_stats);
    Events::subscribe(IEvent::AcceptType, m_shareLog);
    Events::subscribe(IEvent::AcceptType, m_workers);

    m_debug = new ProxyDebug(options->isDebug());
}


Proxy::~Proxy()
{
    Events::stop();

    delete m_miners;
    delete m_splitter;
    delete m_shareLog;
    delete m_accessLog;
    delete m_debug;
    delete m_workers;
}


void Proxy::connect()
{
    m_splitter->connect();

    const std::vector<Addr*> &addrs = Options::i()->addrs();
    for (const Addr *addr : addrs) {
        bind(addr->host(), addr->port());
    }

    uv_timer_start(&m_timer, Proxy::onTick, 1000, 1000);
}


void Proxy::printConnections()
{
    m_splitter->printConnections();
}


void Proxy::printHashrate()
{
    LOG_INFO(Options::i()->colors() ? "\x1B[01;32m* \x1B[01;37mspeed\x1B[0m \x1B[01;30m(1m) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(10m) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(1h) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(12h) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(24h) \x1B[01;36m%03.2f kH/s"
                                    : "* speed (1m) %03.2f, (10m) %03.2f, (1h) %03.2f, (12h) %03.2f, (24h) %03.2f kH/s",
             m_stats.hashrate(60), m_stats.hashrate(600), m_stats.hashrate(3600), m_stats.hashrate(3600 * 12), m_stats.hashrate(3600 * 24));
}


void Proxy::printWorkers()
{
    m_workers->printWorkers();
}


void Proxy::toggleDebug()
{
    m_debug->toggle();
}


#ifdef APP_DEVEL
void Proxy::printState()
{
    LOG_NOTICE("---------------------------------");
    m_splitter->printState();
    LOG_NOTICE("---------------------------------");

    LOG_INFO("%" PRIu64 " (%" PRIu64 ")", Counters::miners(), Counters::connections);
}
#endif


void Proxy::bind(const char *ip, uint16_t port)
{
    auto server = new Server(ip, port);

    if (server->bind()) {
        m_servers.push_back(server);
    }
    else {
        delete server;
    }
}


void Proxy::gc()
{
    m_splitter->gc();
}


void Proxy::print()
{
    LOG_INFO(Options::i()->colors() ? "\x1B[01;36m%03.2f kH/s\x1B[0m, shares: \x1B[01;37m%" PRIu64 "\x1B[0m/%s%" PRIu64 "\x1B[0m +%" PRIu64 ", upstreams: \x1B[01;37m%u\x1B[0m, miners: \x1B[01;37m%" PRIu64 "\x1B[0m (max \x1B[01;37m%" PRIu64 "\x1B[0m) +%u/-%u"
                                    : "%03.2f kH/s, shares: %" PRIu64 "/%s%" PRIu64 " +%" PRIu64 ", upstreams: %u, miners: %" PRIu64 " (max %" PRIu64 " +%u/-%u",
             m_stats.hashrate(60), m_stats.data().accepted, Options::i()->colors() ? (m_stats.data().rejected ? "\x1B[31m" : "\x1B[01;37m") : "", m_stats.data().rejected,
             Counters::accepted, m_splitter->activeUpstreams(), Counters::miners(), Counters::maxMiners(), Counters::added(), Counters::removed());

    Counters::reset();
}


void Proxy::tick()
{
    m_stats.tick(m_ticks, *m_splitter);

    m_ticks++;

    if ((m_ticks % kGCInterval) == 0) {
        gc();
    }

    if ((m_ticks % kPrintInterval) == 0) {
        print();
    }

    m_splitter->tick(m_ticks);
    m_workers->tick(m_ticks);
}


void Proxy::onTick(uv_timer_t *handle)
{
    static_cast<Proxy*>(handle->data)->tick();
}
