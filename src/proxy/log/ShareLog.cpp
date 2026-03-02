/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/Miner.h"
#include "proxy/Stats.h"


#include <cinttypes>


xmrig::ShareLog::ShareLog(Controller *controller, Stats *stats) :
    m_stats(stats),
    m_controller(controller)
{
}


xmrig::ShareLog::~ShareLog() = default;


void xmrig::ShareLog::onEvent(IEvent *event)
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


void xmrig::ShareLog::onRejectedEvent(IEvent *event)
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


void xmrig::ShareLog::accept(const AcceptEvent *event)
{
    if (!m_controller->config()->isVerbose() || event->isDonate() || event->isCustomDiff()) {
        return;
    }

    LOG_INFO("%s " CYAN("%04u ") GREEN_BOLD("accepted") " (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff " WHITE_BOLD("%" PRIu64) " ip " WHITE_BOLD("%s") " " BLACK_BOLD("(%" PRIu64 " ms)"),
             Tags::proxy(), event->mapperId(), m_stats->data().accepted, m_stats->data().rejected, m_stats->data().invalid, event->result.diff, event->ip(), event->result.elapsed);
}


void xmrig::ShareLog::reject(const AcceptEvent *event)
{
    if (event->isDonate()) {
        return;
    }

    LOG_INFO("%s " CYAN("%04u ") RED_BOLD("rejected") " (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff " WHITE_BOLD("%" PRIu64) " ip " WHITE_BOLD("%s") " " RED("\"%s\"") " " BLACK_BOLD("(%" PRIu64 " ms)"),
             Tags::proxy(), event->mapperId(), m_stats->data().accepted, m_stats->data().rejected, m_stats->data().invalid, event->result.diff, event->ip(), event->error(), event->result.elapsed);
}
