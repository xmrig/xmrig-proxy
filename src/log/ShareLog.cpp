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


#include "log/Log.h"
#include "log/ShareLog.h"
#include "net/SubmitResult.h"
#include "Options.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/Stats.h"


ShareLog::ShareLog(const Stats &stats) :
    m_stats(stats)
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


void ShareLog::accept(const AcceptEvent *event)
{
    if (!Options::i()->verbose()) {
        return;
    }

    LOG_INFO(Options::i()->colors() ? "#%03u \x1B[01;32maccepted\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff \x1B[01;37m%u\x1B[0m \x1B[01;30m(%" PRIu64 " ms)"
                                    : "#%03u accepted (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff %u (%" PRIu64 " ms)",
             event->mapperId(), m_stats.data().accepted, m_stats.data().rejected, m_stats.data().invalid, event->result.diff, event->result.elapsed);
}


void ShareLog::reject(const AcceptEvent *event)
{
    if (event->isDonate()) {
        return;
    }

    LOG_INFO(Options::i()->colors() ? "#%03u \x1B[01;31mrejected\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff \x1B[01;37m%u\x1B[0m \x1B[31m\"%s\"\x1B[0m \x1B[01;30m(%" PRId64 " ms)"
                                    : "#%03u rejected (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff %u \"%s\" (%" PRId64 " ms)",
             event->mapperId(), m_stats.data().accepted, m_stats.data().rejected, m_stats.data().invalid, event->result.diff, event->error(), event->result.elapsed);
}
