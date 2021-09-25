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

#include "proxy/Miners.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Miner.h"


namespace xmrig {


class Miners::Private
{
public:
    inline void add(Miner *miner)       { miners.insert({ miner->id(), miner }); }
    inline void remove(Miner *miner)    { miners.erase(miner->id()); }

    void close() {
        for (const auto &s : miners) {
            s.second->close();
        }
    }

    std::map<int64_t, Miner *> miners;
};


} // namespace xmrig


xmrig::Miners::Miners() :
    d(std::make_shared<Private>())
{
}


xmrig::Miners::~Miners()
{
    for (const auto &s : d->miners) {
        delete s.second;
    }
}


std::vector<xmrig::Miner*> xmrig::Miners::miners() const
{
    std::vector<Miner *> miners;
    miners.reserve(d->miners.size());

    for (const auto &kv : d->miners) {
       miners.emplace_back(kv.second);
    }

    return miners;
}


void xmrig::Miners::tick(uint64_t now)
{
    std::vector<Miner *> expired;

    for (auto const &kv : d->miners) {
        if (now > kv.second->expire()) {
            expired.emplace_back(kv.second);
        }
    }

    if (expired.empty()) {
        return;
    }

    for (auto miner : expired) {
        miner->close();
    }
}


void xmrig::Miners::onEvent(uint32_t type, IEvent *event)
{
    if (type == CONNECTION_EVENT && !event->isRejected()) {
        return d->add(static_cast<ConnectionEvent*>(event)->miner());
    }

    if (type == CLOSE_EVENT && !event->isRejected()) {
        return d->remove(static_cast<CloseEvent*>(event)->miner());
    }

    if (type == IEvent::EXIT) {
        return d->close();
    }
}
