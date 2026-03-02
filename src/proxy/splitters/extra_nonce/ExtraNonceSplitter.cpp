/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "proxy/splitters/extra_nonce/ExtraNonceSplitter.h"
#include "base/io/log/Log.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/extra_nonce/ExtraNonceMapper.h"
#include "Summary.h"


#define LABEL(x) " \x1B[01;30m" x ":\x1B[0m "


xmrig::ExtraNonceSplitter* xmrig::ExtraNonceSplitter::Create(Controller* controller)
{
    for (const Pool& pool : controller->config()->pools().data()) {
        if (pool.mode() != Pool::MODE_DAEMON) {
            LOG_ERR("extra_nonce mode can only be used when mining to daemon");
            return nullptr;
        }
    }
    return new ExtraNonceSplitter(controller);
}


xmrig::ExtraNonceSplitter::ExtraNonceSplitter(Controller *controller) : Splitter(controller)
{
}


xmrig::ExtraNonceSplitter::~ExtraNonceSplitter()
{
    delete m_upstream;
}


xmrig::Upstreams xmrig::ExtraNonceSplitter::upstreams() const
{
    return { m_upstream->isActive() ? 1U : 0U, m_upstream->isSuspended() ? 1U : 0U, 1U };
}


void xmrig::ExtraNonceSplitter::connect()
{
    m_upstream = new ExtraNonceMapper(0, m_controller);
    m_upstream->start();
}


void xmrig::ExtraNonceSplitter::gc()
{
    m_upstream->gc();
}


void xmrig::ExtraNonceSplitter::printConnections()
{
    const auto info  = upstreams();
    const auto ratio = info.ratio(Counters::miners());

    LOG_INFO("\x1B[01;32m* \x1B[01;37mupstreams\x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("sleep") "\x1B[01;37m%" PRIu64 "\x1B[0m" LABEL("error") "%s%" PRIu64 "\x1B[0m" LABEL("total") "\x1B[01;37m%" PRIu64,
             info.active ? "\x1B[01;32m" : "\x1B[01;31m", info.active, info.sleep, info.error ? "\x1B[01;31m" : "\x1B[01;37m", info.error, info.total);

    LOG_INFO("\x1B[01;32m* \x1B[01;37mminers   \x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("max") "\x1B[01;37m%" PRIu64 "\x1B[0m" LABEL("ratio") "%s1:%3.1f",
             Counters::miners() ? "\x1B[01;32m" : "\x1B[01;31m", Counters::miners(), Counters::maxMiners(), (ratio > 200 ? "\x1B[01;32m" : "\x1B[01;33m"), ratio);
}


void xmrig::ExtraNonceSplitter::tick(uint64_t ticks)
{
    const uint64_t now = uv_now(uv_default_loop());
    m_upstream->tick(ticks, now);
}


#ifdef APP_DEVEL
void xmrig::ExtraNonceSplitter::printState()
{
    m_upstream->printState();
}
#endif


void xmrig::ExtraNonceSplitter::onConfigChanged(Config *config, Config *previousConfig)
{
    if (config->pools() != previousConfig->pools()) {
        config->pools().print();
        m_upstream->reload(config->pools());
    }
}


void xmrig::ExtraNonceSplitter::onEvent(IEvent *event)
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


void xmrig::ExtraNonceSplitter::login(LoginEvent *event)
{
    if (event->miner()->routeId() != -1) {
        return;
    }

    if (m_upstream->add(event->miner())) {
        return;
    }

    connect();
    login(event);
}


void xmrig::ExtraNonceSplitter::remove(Miner *miner)
{
    if (miner->mapperId() < 0 || miner->routeId() != -1) {
        return;
    }

    m_upstream->remove(miner);
}


void xmrig::ExtraNonceSplitter::submit(SubmitEvent *event)
{
    if (event->miner()->mapperId() < 0 || event->miner()->routeId() != -1) {
        return;
    }

    m_upstream->submit(event);
}
