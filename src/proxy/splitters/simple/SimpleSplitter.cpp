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

#include <inttypes.h>


#include "base/io/log/Log.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/simple/SimpleMapper.h"
#include "proxy/splitters/simple/SimpleSplitter.h"
#include "Summary.h"


#define LABEL(x) " \x1B[01;30m" x ":\x1B[0m "


xmrig::SimpleSplitter::SimpleSplitter(xmrig::Controller *controller) : Splitter(controller),
    m_reuseTimeout(static_cast<uint64_t>(controller->config()->reuseTimeout())),
    m_sequence(0)
{
}


xmrig::SimpleSplitter::~SimpleSplitter()
{
    for (SimpleMapper *mapper : m_released) {
        delete mapper;
    }

    for (auto const &kv : m_upstreams) {
        delete kv.second;
    }
}


xmrig::Upstreams xmrig::SimpleSplitter::upstreams() const
{
    uint64_t active = 0;

    for (auto const &kv : m_upstreams) {
        if (kv.second->isActive()) {
           active++;
        }
    }

    return { active, m_idles.size(), m_upstreams.size() };
}


void xmrig::SimpleSplitter::connect()
{
}


void xmrig::SimpleSplitter::gc()
{
}


void xmrig::SimpleSplitter::printConnections()
{
    const Upstreams info = upstreams();

    LOG_INFO("\x1B[01;32m* \x1B[01;37mupstreams\x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("sleep") "\x1B[01;37m%" PRIu64 "\x1B[0m" LABEL("error") "%s%" PRIu64 "\x1B[0m" LABEL("total") "\x1B[01;37m%" PRIu64,
             info.active ? "\x1B[01;32m" : "\x1B[01;31m", info.active, info.sleep, info.error ? "\x1B[01;31m" : "\x1B[01;37m", info.error, m_upstreams.size());

    LOG_INFO("\x1B[01;32m* \x1B[01;37mminers   \x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("max") "\x1B[01;37m%" PRIu64 "\x1B[0m",
             Counters::miners() ? "\x1B[01;32m" : "\x1B[01;31m", Counters::miners(), Counters::maxMiners());
}


void xmrig::SimpleSplitter::tick(uint64_t ticks)
{
    const uint64_t now = uv_now(uv_default_loop());

    for (SimpleMapper *mapper : m_released) {
        delete mapper;
    }
    m_released.clear();

    for (auto const &kv : m_upstreams) {
        if (kv.second->idleTime() > m_reuseTimeout) {
            m_released.push_back(kv.second);
            continue;
        }

        kv.second->tick(ticks, now);
    }

    if (m_released.empty()) {
        return;
    }

    for (SimpleMapper *mapper : m_released) {
        stop(mapper);
    }
}


#ifdef APP_DEVEL
void xmrig::SimpleSplitter::printState()
{
}
#endif


void xmrig::SimpleSplitter::onConfigChanged(Config *config, Config *previousConfig)
{
    m_reuseTimeout = static_cast<uint64_t>(config->reuseTimeout());

    if (config->pools() != previousConfig->pools()) {
        config->pools().print();

        for (auto const &kv : m_upstreams) {
            kv.second->reload(config->pools());
        }
    }
}


void xmrig::SimpleSplitter::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::CloseType:
        remove(static_cast<CloseEvent*>(event)->miner());
        break;

    case IEvent::LoginType:
        login(static_cast<LoginEvent*>(event));
        break;

    case IEvent::SubmitType:
        submit(static_cast<SubmitEvent*>(event));
        break;

    default:
        break;
    }
}


void xmrig::SimpleSplitter::login(LoginEvent *event)
{
    if (event->miner()->routeId() != -1) {
        return;
    }

    if (!m_idles.empty()) {
        for (auto const &kv : m_idles) {
            if (kv.second->isReusable()) {
                removeIdle(kv.first);
                kv.second->reuse(event->miner());

                return;
            }
        }
    }

    SimpleMapper *mapper = new SimpleMapper(m_sequence++, m_controller);
    m_upstreams[mapper->id()] = mapper;

    mapper->add(event->miner());
}


void xmrig::SimpleSplitter::stop(SimpleMapper *mapper)
{
    removeIdle(mapper->id());
    removeUpstream(mapper->id());

    mapper->stop();
}


void xmrig::SimpleSplitter::remove(Miner *miner)
{
    const size_t id = static_cast<size_t>(miner->mapperId());
    if (miner->mapperId() < 0 || miner->routeId() != -1 || m_upstreams.count(id) == 0) {
        return;
    }

    SimpleMapper *mapper = m_upstreams[id];
    mapper->remove(miner);

    if (m_reuseTimeout == 0) {
        stop(mapper);

        m_released.push_back(mapper);
    }
    else {
        m_idles[id] = mapper;
    }
}


void xmrig::SimpleSplitter::removeIdle(uint64_t id)
{
    auto it = m_idles.find(id);
    if (it != m_idles.end()) {
        m_idles.erase(it);
    }
}


void xmrig::SimpleSplitter::removeUpstream(uint64_t id)
{
    auto it = m_upstreams.find(id);
    if (it != m_upstreams.end()) {
        m_upstreams.erase(it);
    }
}


void xmrig::SimpleSplitter::submit(SubmitEvent *event)
{
    if (event->miner()->mapperId() < 0 || event->miner()->routeId() != -1) {
        return;
    }

    SimpleMapper *mapper = m_upstreams[event->miner()->mapperId()];
    if (mapper) {
        mapper->submit(event);
    }
}
