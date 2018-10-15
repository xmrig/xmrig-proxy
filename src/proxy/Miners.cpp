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


#include <vector>


#include "common/log/Log.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Miner.h"
#include "proxy/Miners.h"


Miners::Miners()
{
    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);
    uv_timer_start(&m_timer, [](uv_timer_t *handle) { static_cast<Miners*>(handle->data)->tick(); }, kTickInterval, kTickInterval);
}


Miners::~Miners()
{
}


std::vector<Miner*> Miners::miners() const
{
    std::vector<Miner*> miners;
    miners.reserve(m_miners.size());

    for (const auto &s : m_miners) {
       miners.push_back(s.second);
    }

    return miners;
}


void Miners::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::ConnectionType:
        add(static_cast<ConnectionEvent*>(event)->miner());
        break;

    case IEvent::CloseType:
        remove(static_cast<CloseEvent*>(event)->miner());
        break;

    default:
        break;
    }
}


void Miners::add(Miner *miner)
{
    m_miners[miner->id()] = miner;
}


void Miners::remove(Miner *miner)
{
    auto it = m_miners.find(miner->id());
    if (it != m_miners.end()) {
        m_miners.erase(it);
    }
}


void Miners::tick()
{
    const uint64_t now = uv_now(uv_default_loop());
    std::vector<Miner*> expired;

    for (auto const &kv : m_miners) {
        if (now > kv.second->expire()) {
            expired.push_back(kv.second);
        }
    }

    if (expired.empty()) {
        return;
    }

    for (auto miner : expired) {
        miner->close();
    }
}
