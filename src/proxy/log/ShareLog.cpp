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

#include "proxy/log/ShareLog.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/net/stratum/SubmitResult.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/Miner.h"
#include "proxy/Stats.h"
#include "proxy/StatsData.h"


#include <cinttypes>


void xmrig::ShareLog::onEvent(uint32_t type, IEvent *event)
{
    if (type != ACCEPT_EVENT) {
        return;
    }

    const auto *e = static_cast<const AcceptEvent*>(event);

    if (!event->isRejected()) {
        accept(e);
    }
    else {
        reject(e);
    }
}


void xmrig::ShareLog::accept(const AcceptEvent *event)
{
    if (!Log::isVerbose() || event->isDonate() || event->isCustomDiff()) {
        return;
    }

    LOG_V1("%s " CYAN("%04u ") GREEN_BOLD("accepted") " (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff " WHITE_BOLD("%" PRIu64) " ip " WHITE_BOLD("%s") " " BLACK_BOLD("(%" PRIu64 " ms)"),
           Tags::proxy(), event->mapperId(), m_stats->data().accepted, m_stats->data().rejected, m_stats->data().invalid, event->result.diff, event->ip(), event->result.elapsed);
}


void xmrig::ShareLog::reject(const AcceptEvent *event)
{
    if (event->isDonate()) {
        return;
    }

    LOG_WARN("%s " CYAN("%04u ") RED_BOLD("rejected") " (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff " WHITE_BOLD("%" PRIu64) " ip " WHITE_BOLD("%s") " " RED("\"%s\"") " " BLACK_BOLD("(%" PRIu64 " ms)"),
             Tags::proxy(), event->mapperId(), m_stats->data().accepted, m_stats->data().rejected, m_stats->data().invalid, event->result.diff, event->ip(), event->error(), event->result.elapsed);
}
