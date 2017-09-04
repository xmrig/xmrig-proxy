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

#include <assert.h>
#include <inttypes.h>


#include "log/Log.h"
#include "Options.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/NonceMapper.h"
#include "proxy/splitters/NonceSplitter.h"


#define LABEL(x) " \x1B[01;30m" x ":\x1B[0m "


NonceSplitter::NonceSplitter(const Options *options, const char *agent) :
    m_agent(agent),
    m_options(options)
{
    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    uv_timer_start(&m_timer, NonceSplitter::onTick, kTickInterval, kTickInterval);
}


NonceSplitter::~NonceSplitter()
{
}


void NonceSplitter::connect()
{
    auto upstream = new NonceMapper(m_upstreams.size(), m_options, m_agent);
    m_upstreams.push_back(upstream);

    upstream->connect();
}


void NonceSplitter::gc()
{
    for (NonceMapper *mapper : m_upstreams) {
        mapper->gc();
    }
}


void NonceSplitter::printConnections()
{
    int active    = 0;
    int suspended = 0;

    for (NonceMapper *mapper : m_upstreams) {
        if (mapper->isActive()) {
            active++;
            continue;
        }

        if (mapper->isSuspended()) {
            suspended++;
            continue;
        }
    }

    const int error = (int) m_upstreams.size() - active - suspended;
    double efficiency = (double) Counters::miners() / (active * 256) * 100.0;

    if (m_options->colors()) {
        LOG_INFO("\x1B[01;32m* \x1B[01;37mupstreams\x1B[0m" LABEL("active") "%s%d\x1B[0m" LABEL("sleep") "\x1B[01;37m%d\x1B[0m" LABEL("error") "%s%d\x1B[0m" LABEL("total") "\x1B[01;37m%d",
                 active ? "\x1B[01;32m" : "\x1B[01;31m", active, suspended, error ? "\x1B[01;31m" : "\x1B[01;37m", error, m_upstreams.size());

        LOG_INFO("\x1B[01;32m* \x1B[01;37mminers   \x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("max") "\x1B[01;37m%" PRIu64 "\x1B[0m" LABEL("efficiency") "%s%3.1f%%",
                 Counters::miners() ? "\x1B[01;32m" : "\x1B[01;31m", Counters::miners(), Counters::minersMax(), (efficiency > 80.0 ? "\x1B[01;32m" : (efficiency < 30.0 ? "\x1B[01;31m" : "\x1B[01;33m")), efficiency);
    }
    else {
        LOG_INFO("* upstreams: active %d sleep %d error %d total %d",
                 active, suspended, error, m_upstreams.size());

        LOG_INFO("* miners:    active %" PRIu64 " max %" PRIu64 " efficiency %3.1f%%",
                 Counters::miners(), Counters::minersMax(), efficiency);
    }
}


#ifdef APP_DEVEL
void NonceSplitter::printState()
{
    for (NonceMapper *mapper : m_upstreams) {
        mapper->printState();
    }
}
#endif


void NonceSplitter::onEvent(IEvent *event)
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


void NonceSplitter::onTick(uv_timer_t *handle)
{
    static_cast<NonceSplitter*>(handle->data)->tick();
}


void NonceSplitter::login(LoginEvent *event)
{
    // try reuse active upstreams.
    for (NonceMapper *mapper : m_upstreams) {
        if (!mapper->isSuspended() && mapper->add(event->miner(), event->request)) {
            return;
        }
    }

    // try reuse suspended upstreams.
    for (NonceMapper *mapper : m_upstreams) {
        if (mapper->isSuspended() && mapper->add(event->miner(), event->request)) {
            return;
        }
    }

    connect();
    login(event);
}


void NonceSplitter::remove(Miner *miner)
{
    if (miner->mapperId() < 0) {
        return;
    }

    m_upstreams[miner->mapperId()]->remove(miner);
}


void NonceSplitter::submit(SubmitEvent *event)
{
    assert(event->miner()->mapperId() >= 0);

    m_upstreams[event->miner()->mapperId()]->submit(event->miner(), event->request);
}


void NonceSplitter::tick()
{
    const uint64_t now = uv_now(uv_default_loop());

    for (NonceMapper *mapper : m_upstreams) {
        mapper->tick(now);
    }
}
