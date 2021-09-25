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

#include "proxy/ProxyDebug.h"
#include "base/io/log/Log.h"
#include "base/net/stratum/SubmitResult.h"
#include "net/JobResult.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"


#include <cinttypes>


namespace xmrig {


static const char *tag = "[debug  ]";
static inline Log::Level level(const IEvent *event) { return event->isRejected() ? Log::ERR : Log::V5; }


static inline void print(const ConnectionEvent *e)
{
    Log::print(level(e), "%s connection <Miner id=%" PRId64 ", ip=%s> via port: %d", tag, e->miner()->id(), e->miner()->ip(), e->port());
}


static inline void print(const CloseEvent *e)
{
    Log::print(level(e), "%s close <Miner id=%" PRId64 ", ip=%s>", tag, e->miner()->id(), e->miner()->ip());
}


static inline void print(const LoginEvent *e)
{
    Log::print(level(e), "%s login <Miner id=%" PRId64 ", ip=%s>, <Request login=%s, agent=%s>",
               tag, e->miner()->id(), e->miner()->ip(), e->miner()->user().data(), e->miner()->agent().data());
}


static inline void print(const SubmitEvent *e)
{
    if (e->isRejected()) {
        LOG_ERR("%s submit <Miner id=%" PRId64 ", ip=%s>, message=%s", tag, e->miner()->id(), e->miner()->ip(), e->message());
    }
    else {
        LOG_V5("%s submit <Miner id=%" PRId64 ", ip=%s>, <Job actualDiff=%" PRIu64 ">", tag, e->miner()->id(), e->miner()->ip(), e->request.actualDiff());
    }
}


static inline void print(const AcceptEvent *e)
{
    const int64_t id = e->miner() ? e->miner()->id() : -1;
    const char *ip   = e->miner() ? e->miner()->ip() : "?.?.?.?";

    if (e->isRejected()) {
        LOG_ERR("%s rejected <Miner id=%" PRId64 ", ip=%s>, <Result diff=%" PRIu64 ", elapsed=%" PRIu64 ">, error=%s",
                 tag, id, ip, e->result.diff, e->result.elapsed, e->error());
    }
    else {
        LOG_V5("%s accepted <Miner id=%" PRId64 ", ip=%s>, <Result diff=%" PRIu64 ", actualDiff=%" PRIu64 ", elapsed=%" PRIu64 ">",
               tag, id, ip, e->result.diff, e->result.actualDiff, e->result.elapsed);
    }
}


} // namespace xmrig


void xmrig::ProxyDebug::onEvent(uint32_t type, IEvent *event)
{
    if (Log::verbose() < 5) {
        return;
    }

    switch (type) {
    case CONNECTION_EVENT:
        return print(static_cast<const ConnectionEvent *>(event));

    case CLOSE_EVENT:
        return print(static_cast<const CloseEvent *>(event));

    case LOGIN_EVENT:
        return print(static_cast<const LoginEvent *>(event));

    case SUBMIT_EVENT:
        return print(static_cast<const SubmitEvent *>(event));

    case ACCEPT_EVENT:
        return print(static_cast<const AcceptEvent *>(event));

    default:
        break;
    }
}
