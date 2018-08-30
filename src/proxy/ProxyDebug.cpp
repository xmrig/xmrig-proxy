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


#include <inttypes.h>


#include "common/log/Log.h"
#include "common/net/SubmitResult.h"
#include "net/JobResult.h"
#include "proxy/Events.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/ProxyDebug.h"


ProxyDebug::ProxyDebug(bool enabled) :
    m_enabled(enabled)
{
    Events::subscribe(IEvent::ConnectionType, this);
    Events::subscribe(IEvent::CloseType, this);
    Events::subscribe(IEvent::LoginType, this);
    Events::subscribe(IEvent::SubmitType, this);
    Events::subscribe(IEvent::AcceptType, this);
}


ProxyDebug::~ProxyDebug()
{
}


void ProxyDebug::onEvent(IEvent *event)
{
    if (!m_enabled) {
        return;
    }

    switch (event->type())
    {
    case IEvent::ConnectionType: {
            auto e = static_cast<ConnectionEvent*>(event);
            LOG_INFO("[debug] connection <Miner id=%" PRId64 ", ip=%s> via port: %d", e->miner()->id(), e->miner()->ip(), e->port());
        }
        break;

    case IEvent::LoginType: {
            auto e = static_cast<LoginEvent*>(event);
            LOG_INFO("[debug] login <Miner id=%" PRId64 ", ip=%s>, <Request login=%s, agent=%s>", e->miner()->id(), e->miner()->ip(), e->miner()->user(), e->miner()->agent());
        }
        break;

    case IEvent::CloseType: {
            auto e = static_cast<CloseEvent*>(event);
            LOG_INFO("[debug] close <Miner id=%" PRId64 ", ip=%s>", e->miner()->id(), e->miner()->ip());
        }
        break;

    case IEvent::SubmitType: {
            auto e = static_cast<SubmitEvent*>(event);
            LOG_INFO("[debug] submit <Miner id=%" PRId64 ", ip=%s>, <Job actualDiff=%" PRIu64 ">", e->miner()->id(), e->miner()->ip(), e->request.actualDiff());
        }
        break;

    case IEvent::AcceptType: {
            auto e = static_cast<AcceptEvent*>(event);
            LOG_INFO("[debug] accepted <Miner id=%" PRId64 ", ip=%s>, <Result diff=%u, actualDiff=%" PRIu64 ", elapsed=%" PRIu64 ">",
                     e->miner() ? e->miner()->id() : -1, e->miner() ? e->miner()->ip() : "?.?.?.?", e->result.diff, e->result.actualDiff, e->result.elapsed);
        }
        break;


    default:
        break;
    }
}


void ProxyDebug::onRejectedEvent(IEvent *event)
{
    if (!m_enabled) {
        return;
    }

    switch (event->type())
    {
    case IEvent::ConnectionType: {
            ConnectionEvent *e = static_cast<ConnectionEvent*>(event);
            LOG_ERR("[error] connection <Miner id=%" PRId64 ", ip=%s> via port: %d", e->miner()->id(), e->miner()->ip(), e->port());
        }
        break;

    case IEvent::LoginType: {
            auto e = static_cast<LoginEvent*>(event);
            LOG_ERR("[error] login <Miner id=%" PRId64 ", ip=%s>, <Request login=%s, agent=%s>", e->miner()->id(), e->miner()->ip(), e->miner()->user(), e->miner()->agent());
        }
        break;

    case IEvent::CloseType: {
            auto e = static_cast<CloseEvent*>(event);
            LOG_ERR("[error] close <Miner id=%" PRId64 ", ip=%s>", e->miner()->id(), e->miner()->ip());
        }
        break;

    case IEvent::SubmitType: {
            auto e = static_cast<SubmitEvent*>(event);
            LOG_ERR("[error] submit <Miner id=%" PRId64 ", ip=%s>, message=%s", e->miner()->id(), e->miner()->ip(), e->message());
        }
        break;

    case IEvent::AcceptType: {
            auto e = static_cast<AcceptEvent*>(event);
            LOG_ERR("[error] rejected <Miner id=%" PRId64 ", ip=%s>, <Result diff=%u, elapsed=%" PRIu64 ">, error=%s",
                     e->miner() ? e->miner()->id() : -1, e->miner() ? e->miner()->ip() : "?.?.?.?", e->result.diff, e->result.elapsed, e->error());
        }
        break;

    default:
        break;
    }
}
