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

#include "proxy/splitters/extra_nonce/ExtraNonceSplitter.h"
#include "base/io/log/Log.h"
#include "core/Controller.h"
#include "proxy/config/MainConfig.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/extra_nonce/ExtraNonceMapper.h"
#include "Summary.h"


#include <stdexcept>


#define LABEL(x) " \x1B[01;30m" x ":\x1B[0m "


std::shared_ptr<xmrig::ExtraNonceSplitter> xmrig::ExtraNonceSplitter::create(Controller *controller)
{
    for (const Pool& pool : controller->config()->pools().data()) {
        if ((pool.algorithm() == Algorithm::ASTROBWT_DERO) || (pool.coin() == Coin::DERO)) {
            throw std::runtime_error("extra_nonce mode is incompatible with Dero mining");
        }

        if (pool.mode() != Pool::MODE_DAEMON) {
            throw std::runtime_error("extra_nonce mode can only be used when mining to daemon");
        }
    }

    return std::shared_ptr<ExtraNonceSplitter>(new ExtraNonceSplitter(controller));
}


xmrig::ExtraNonceSplitter::ExtraNonceSplitter(Controller *controller) : Splitter(controller)
{
}


xmrig::Upstreams xmrig::ExtraNonceSplitter::upstreams() const
{
    return { m_upstream->isActive() ? 1U : 0U, m_upstream->isSuspended() ? 1U : 0U, 1U };
}


void xmrig::ExtraNonceSplitter::connect()
{
    m_upstream = std::make_shared<ExtraNonceMapper>(0, m_controller);
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


void xmrig::ExtraNonceSplitter::tick(uint64_t ticks, uint64_t now)
{
    m_upstream->tick(ticks, now);
}


#ifdef APP_DEVEL
void xmrig::ExtraNonceSplitter::printState()
{
    m_upstream->printState();
}
#endif


void xmrig::ExtraNonceSplitter::onEvent(uint32_t type, IEvent *event)
{
    if (event->isRejected()) {
        return;
    }

    switch (type) {
    case CLOSE_EVENT:
        return remove(static_cast<const CloseEvent *>(event)->miner());

    case LOGIN_EVENT:
        return login(static_cast<const LoginEvent *>(event));

    case SUBMIT_EVENT:
        return submit(static_cast<SubmitEvent *>(event));

    default:
        break;
    }
}


void xmrig::ExtraNonceSplitter::login(const LoginEvent *event)
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
