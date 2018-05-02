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


#include <inttypes.h>


#include "common/log/Log.h"
#include "common/net/SubmitResult.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "log/ShareLog.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/Miner.h"
#include "proxy/Stats.h"


ShareLog::ShareLog(xmrig::Controller *controller, const Stats &stats) :
    m_stats(stats),
    m_controller(controller)
{
}


ShareLog::~ShareLog()
{
}


void ShareLog::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::AcceptType:
        accept(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


void ShareLog::onRejectedEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::AcceptType:
        reject(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


bool ShareLog::isColors() const
{
    return m_controller->config()->isColors();
}


void ShareLog::accept(const AcceptEvent *event)
{
    if (!m_controller->config()->isVerbose()) {
        return;
    }

    LOG_INFO(isColors() ? "#%03u \x1B[01;32maccepted\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff \x1B[01;37m%u\x1B[0m ip \x1B[01;37m%s \x1B[01;30m(%" PRIu64 " ms)"
                        : "#%03u accepted (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff %u ip %s (%" PRIu64 " ms)",
             event->mapperId(), m_stats.data().accepted, m_stats.data().rejected, m_stats.data().invalid, event->result.diff, event->ip(), event->result.elapsed);
}


void ShareLog::reject(const AcceptEvent *event)
{
    if (event->isDonate()) {
        return;
    }

    LOG_INFO(isColors() ? "#%03u \x1B[01;31mrejected\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff \x1B[01;37m%u\x1B[0m ip \x1B[01;37m%s \x1B[31m\"%s\"\x1B[0m \x1B[01;30m(%" PRId64 " ms)"
                        : "#%03u rejected (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff %u ip %s \"%s\" (%" PRId64 " ms)",
             event->mapperId(), m_stats.data().accepted, m_stats.data().rejected, m_stats.data().invalid, event->result.diff, event->ip(), event->error(), event->result.elapsed);
}
