/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include <cinttypes>
#include <memory>
#include <ctime>


#include "proxy/Proxy.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/tools/Handle.h"
#include "base/tools/Timer.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "Counters.h"
#include "log/AccessLog.h"
#include "log/ShareLog.h"
#include "proxy/Events.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Login.h"
#include "proxy/Miner.h"
#include "proxy/Miners.h"
#include "proxy/ProxyDebug.h"
#include "proxy/Server.h"
#include "proxy/splitters/donate/DonateSplitter.h"
#include "proxy/splitters/extra_nonce/ExtraNonceSplitter.h"
#include "proxy/splitters/nicehash/NonceSplitter.h"
#include "proxy/splitters/simple/SimpleSplitter.h"
#include "proxy/Stats.h"
#include "proxy/workers/Workers.h"


#ifdef XMRIG_FEATURE_TLS
#   include "base/net/tls/TlsContext.h"
#endif


#ifdef XMRIG_FEATURE_API
#   include "api/v1/ApiRouter.h"
#   include "base/api/Api.h"
#endif


xmrig::Proxy::Proxy(Controller *controller) :
    m_controller(controller),
    m_customDiff(controller)
{
    m_miners = new Miners();
    m_login  = new Login(controller);

    Splitter *splitter = nullptr;
    if (controller->config()->mode() == Config::NICEHASH_MODE) {
        splitter = new NonceSplitter(controller);
    }
    else if (controller->config()->mode() == Config::EXTRA_NONCE_MODE) {
        splitter = ExtraNonceSplitter::Create(controller);
        if (!splitter) {
            LOG_WARN("Switching to nicehash mode");
            splitter = new NonceSplitter(controller);
        }
    }
    else {
        splitter = new SimpleSplitter(controller);
    }

    m_splitter  = splitter;
    m_donate    = new DonateSplitter(controller);
    m_stats     = new Stats(controller);
    m_shareLog  = new ShareLog(controller, m_stats);
    m_accessLog = new AccessLog(controller);
    m_workers   = new Workers(controller);

    m_timer = new Timer(this);

#   ifdef XMRIG_FEATURE_API
    m_api = new ApiRouter(controller);
    controller->api()->addListener(m_api);
#   endif

    Events::subscribe(IEvent::ConnectionType, m_miners);
    Events::subscribe(IEvent::ConnectionType, m_stats);

    Events::subscribe(IEvent::CloseType, m_miners);
    Events::subscribe(IEvent::CloseType, m_donate);
    Events::subscribe(IEvent::CloseType, splitter);
    Events::subscribe(IEvent::CloseType, m_stats);
    Events::subscribe(IEvent::CloseType, m_accessLog);
    Events::subscribe(IEvent::CloseType, m_workers);

    Events::subscribe(IEvent::LoginType, m_login);
    Events::subscribe(IEvent::LoginType, m_donate);
    Events::subscribe(IEvent::LoginType, &m_customDiff);
    Events::subscribe(IEvent::LoginType, splitter);
    Events::subscribe(IEvent::LoginType, m_stats);
    Events::subscribe(IEvent::LoginType, m_accessLog);
    Events::subscribe(IEvent::LoginType, m_workers);

    Events::subscribe(IEvent::SubmitType, m_donate);
    Events::subscribe(IEvent::SubmitType, splitter);
    Events::subscribe(IEvent::SubmitType, m_stats);
    Events::subscribe(IEvent::SubmitType, m_workers);

    Events::subscribe(IEvent::AcceptType, m_stats);
    Events::subscribe(IEvent::AcceptType, m_shareLog);
    Events::subscribe(IEvent::AcceptType, m_workers);

    m_debug = new ProxyDebug(controller->config()->isDebug());

    controller->addListener(this);
}


xmrig::Proxy::~Proxy()
{
    Events::stop();

    delete m_timer;

    for (Server *server : m_servers) {
        delete server;
    }

#   ifdef XMRIG_FEATURE_API
    delete m_api;
#   endif

    delete m_donate;
    delete m_login;
    delete m_miners;
    delete m_splitter;
    delete m_stats;
    delete m_shareLog;
    delete m_accessLog;
    delete m_debug;
    delete m_workers;

#   ifdef XMRIG_FEATURE_TLS
    delete m_tls;
#   endif
}


void xmrig::Proxy::connect()
{
#   ifdef XMRIG_FEATURE_TLS
    m_tls = TlsContext::create(m_controller->config()->tls());
#   endif

    m_splitter->connect();

    const BindHosts &bind = m_controller->config()->bind();
    for (const BindHost &host : bind) {
        this->bind(host);
    }

    m_timer->start(1000, 1000);
}


void xmrig::Proxy::printConnections()
{
    m_splitter->printConnections();
}


void xmrig::Proxy::printHashrate()
{
    LOG_INFO("%s \x1B[01;37mspeed\x1B[0m \x1B[01;30m(1m) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(10m) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(1h) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(12h) \x1B[01;36m%03.2f\x1B[0m, \x1B[01;30m(24h) \x1B[01;36m%03.2f kH/s",
             Tags::proxy(), m_stats->hashrate(60), m_stats->hashrate(600), m_stats->hashrate(3600), m_stats->hashrate(3600 * 12), m_stats->hashrate(3600 * 24));
}


void xmrig::Proxy::printWorkers()
{
    m_workers->printWorkers();
}


void xmrig::Proxy::toggleDebug()
{
    m_debug->toggle();
}


const xmrig::StatsData &xmrig::Proxy::statsData() const
{
    return m_stats->data();
}


const std::vector<xmrig::Worker> &xmrig::Proxy::workers() const
{
    return m_workers->workers();
}


std::vector<xmrig::Miner*> xmrig::Proxy::miners() const
{
    return m_miners->miners();
}


#ifdef APP_DEVEL
void xmrig::Proxy::printState()
{
    LOG_NOTICE("---------------------------------");
    m_splitter->printState();
    LOG_NOTICE("---------------------------------");

    LOG_INFO("%" PRIu64 " (%" PRIu64 ")", Counters::miners(), Counters::connections);
}
#endif


void xmrig::Proxy::onConfigChanged(xmrig::Config *config, xmrig::Config *)
{
    m_debug->setEnabled(config->isDebug());
}


void xmrig::Proxy::bind(const xmrig::BindHost &host)
{
#   ifdef XMRIG_FEATURE_TLS
    if (host.isTLS() && !m_tls) {
        LOG_ERR("Failed to bind \"%s:%d\" error: \"TLS not available\".", host.host(), host.port());

        return;
    }
#   endif

    auto server = new Server(host, m_tls);

    if (server->bind()) {
        m_servers.push_back(server);
    }
    else {
        delete server;
    }
}


void xmrig::Proxy::gc()
{
    m_splitter->gc();
}


void xmrig::Proxy::print()
{
    LOG_INFO("%s \x1B[01;36m%03.2f kH/s\x1B[0m, shares: \x1B[01;37m%" PRIu64 "\x1B[0m/%s%" PRIu64 "\x1B[0m +%" PRIu64 ", upstreams: \x1B[01;37m%" PRIu64 "\x1B[0m, miners: \x1B[01;37m%" PRIu64 "\x1B[0m (max \x1B[01;37m%" PRIu64 "\x1B[0m) +%u/-%u",
             Tags::proxy(), m_stats->hashrate(m_controller->config()->printTime()), m_stats->data().accepted, (m_stats->data().rejected ? "\x1B[0;31m" : "\x1B[1;37m"), m_stats->data().rejected,
             Counters::accepted, m_splitter->upstreams().active, Counters::miners(), Counters::maxMiners(), Counters::added(), Counters::removed());

    Counters::reset();
}


void xmrig::Proxy::tick()
{
    m_stats->tick(m_ticks, m_splitter);

    m_ticks++;

    if ((m_ticks % kGCInterval) == 0) {
        gc();
    }

    auto seconds = m_controller->config()->printTime();
    if (seconds && (m_ticks % seconds) == 0) {
        print();
    }

    m_splitter->tick(m_ticks);
    m_workers->tick(m_ticks);
}
