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

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#include <cinttypes>
#include <memory>
#include <ctime>


#include "proxy/Proxy.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/App.h"
#include "base/kernel/Events.h"
#include "base/kernel/events/IdleEvent.h"
#include "base/kernel/interfaces/ITimerListener.h"
#include "base/kernel/Process.h"
#include "base/tools/Handle.h"
#include "base/tools/Timer.h"
#include "core/Controller.h"
#include "Counters.h"
#include "log/AccessLog.h"
#include "log/ShareLog.h"
#include "proxy/BindHost.h"
#include "proxy/config/MainConfig.h"
#include "proxy/CustomDiff.h"
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


#ifdef XMRIG_FEATURE_TLS
#   include "base/net/tls/TlsContext.h"
#endif


#ifdef XMRIG_FEATURE_API
#   include "api/v1/ApiRouter.h"
#   include "base/api/Api.h"
#endif


namespace xmrig {


class Proxy::Private : public ITimerListener
{
public:
    constexpr static uint64_t kGCInterval   = 60;

    inline void gc()        { splitter->gc(); }

    void bind(const BindHost &host);
    void close();
    void connect();
    void print();
    void tick();

#   ifdef APP_DEVEL
    void printState();
#   endif

    bool ready              = false;
    Controller *controller  = nullptr;
    std::shared_ptr<AccessLog> access;
    std::shared_ptr<CustomDiff> diff;
    std::shared_ptr<DonateSplitter> donate;
    std::shared_ptr<Login> login;
    std::shared_ptr<Miners> miners;
    std::shared_ptr<ProxyDebug> debug;
    std::shared_ptr<ShareLog> shareLog;
    std::shared_ptr<Splitter> splitter;
    std::shared_ptr<Stats> stats;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<TlsContext> tls;
    std::vector<std::shared_ptr<Server> > servers;
    uint64_t ticks          = 0;

protected:
    inline void onTimer(const Timer * /*timer*/) override   { tick(); }
};


} // namespace xmrig


xmrig::Proxy::Proxy(Controller *controller, const ConfigEvent *event) :
    d(std::make_shared<Private>())
{
    d->controller   = controller;
    d->miners       = std::make_shared<Miners>();
    d->login        = std::make_shared<Login>(event);
    d->diff         = std::make_shared<CustomDiff>(event);
    d->donate       = std::make_shared<DonateSplitter>(controller);

    if (controller->config()->mode() == MainConfig::NICEHASH_MODE) {
        d->splitter = std::make_shared<NonceSplitter>(controller, event);
    }
    else if (controller->config()->mode() == MainConfig::EXTRA_NONCE_MODE) {
        try {
            d->splitter = ExtraNonceSplitter::create(controller);
        } catch (std::exception &ex) {
            LOG_ERR("%s " RED_BOLD("%s"), Tags::proxy(), ex.what());
            LOG_WARN("%s " YELLOW("switching to nicehash mode"), Tags::proxy());

            d->splitter = std::make_shared<NonceSplitter>(controller, event);
        }
    }
    else {
        d->splitter = std::make_shared<SimpleSplitter>(controller, event);
    }

    d->stats        = std::make_shared<Stats>(event);
    d->shareLog     = std::make_shared<ShareLog>(d->stats);
    d->access       = std::make_shared<AccessLog>(event);
    d->debug        = std::make_shared<ProxyDebug>();

    Process::events().post<IdleEvent>();
}


const xmrig::StatsData &xmrig::Proxy::statsData() const
{
    return d->stats->data();
}


std::vector<xmrig::Miner*> xmrig::Proxy::miners() const
{
    return d->miners->miners();
}


void xmrig::Proxy::onEvent(uint32_t type, IEvent *event)
{
    if (type == IEvent::PRINT) {
        return d->print();
    }

    if (type == IEvent::IDLE && !d->ready) {
        return d->connect();
    }

    if (type == IEvent::EXIT) {
        return d->close();
    }

    if (type == IEvent::CONSOLE && (event->data() == 'c' || event->data() == 'C')) {
        return d->splitter->printConnections();
    }

#   ifdef APP_DEVEL
    if (type == IEvent::CONSOLE && (event->data() == 's' || event->data() == 'S')) {
        return d->printState();
    }
#   endif
}


void xmrig::Proxy::Private::bind(const BindHost &host)
{
#   ifdef XMRIG_FEATURE_TLS
    if (host.isTLS() && !tls) {
        LOG_ERR("Failed to bind \"%s:%d\" error: \"TLS not available\".", host.host(), host.port());

        return;
    }
#   endif

    auto server = std::make_shared<Server>(host, tls.get());
    if (server->bind()) {
        servers.emplace_back(std::move(server));
    }
}


void xmrig::Proxy::Private::close()
{
    timer.reset();
    servers.clear();
    splitter.reset();
}


void xmrig::Proxy::Private::connect()
{
#   ifdef XMRIG_FEATURE_TLS
    tls = std::shared_ptr<TlsContext>(TlsContext::create(controller->config()->tls()));
#   endif

    splitter->connect();

    const auto &hosts = controller->config()->bind();

    if (!hosts.empty()) {
        servers.reserve(hosts.size());

        for (const auto &host : hosts) {
            bind(host);
        }
    }

    if (servers.empty()) {
        Process::exit(1);

        return;
    }

    timer = std::make_shared<Timer>(this, 1000, 1000);
    ready = true;
}


void xmrig::Proxy::Private::print()
{
    LOG_INFO("%s \x1B[01;36m%03.2f kH/s\x1B[0m, shares: \x1B[01;37m%" PRIu64 "\x1B[0m/%s%" PRIu64 "\x1B[0m +%" PRIu64 ", upstreams: \x1B[01;37m%" PRIu64 "\x1B[0m, miners: \x1B[01;37m%" PRIu64 "\x1B[0m (max \x1B[01;37m%" PRIu64 "\x1B[0m) +%u/-%u",
             Tags::proxy(), stats->hashrate(60), stats->data().accepted, (stats->data().rejected ? "\x1B[0;31m" : "\x1B[1;37m"), stats->data().rejected,
             Counters::accepted, splitter->upstreams().active, Counters::miners(), Counters::maxMiners(), Counters::added(), Counters::removed());

    Counters::reset();
}


void xmrig::Proxy::Private::tick()
{
    const uint64_t now = Chrono::steadyMSecs();
    stats->tick(ticks, splitter.get());

    ticks++;

    if ((ticks % kGCInterval) == 0) {
        gc();
    }

    splitter->tick(ticks, now);
    miners->tick(now);
}


#ifdef APP_DEVEL
void xmrig::Proxy::Private::printState()
{
    LOG_NOTICE("---------------------------------");
    splitter->printState();
    LOG_NOTICE("---------------------------------");

    LOG_INFO("%" PRIu64 " (%" PRIu64 ")", Counters::miners(), Counters::connections);
}
#endif
