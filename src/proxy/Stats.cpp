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


#include "log/Log.h"

#include "api/Api.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Stats.h"


Stats::Stats()
{
}


Stats::~Stats()
{
}


void Stats::tick(uint64_t ticks)
{
    ticks++;

#   ifndef XMRIG_NO_API
    Api::tick(m_data);
#   endif
}


void Stats::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::ConnectionType:
        m_data.connections++;
        m_data.miners++;

        if (m_data.miners > m_data.maxMiners) {
            m_data.maxMiners = m_data.miners;
        }
        break;

    case IEvent::CloseType:
        m_data.connections--;
        m_data.miners--;
        break;

    default:
        break;
    }
}


void Stats::onRejectedEvent(IEvent *event)
{
}
