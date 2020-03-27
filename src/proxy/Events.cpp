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


#include "base/io/log/Log.h"
#include "proxy/Events.h"


namespace xmrig {

bool Events::m_ready = true;
std::map<IEvent::Type, std::vector<IEventListener*> > Events::m_listeners;

}


bool xmrig::Events::exec(IEvent *event)
{
    if (!m_ready) {
        LOG_ERR("failed start event %d", (int) event->type());
        return false;
    }

    m_ready = false;

    std::vector<IEventListener*> &listeners = m_listeners[event->type()];
    for (IEventListener *listener : listeners) {
        event->isRejected() ? listener->onRejectedEvent(event) : listener->onEvent(event);
    }

    const bool rejected = event->isRejected();
    event->~IEvent();

    m_ready = true;
    return !rejected;
}


void xmrig::Events::stop()
{
    m_listeners.clear();
}


void xmrig::Events::subscribe(IEvent::Type type, IEventListener *listener)
{
    m_listeners[type].push_back(listener);
}
