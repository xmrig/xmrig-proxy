/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_ISPLITTER_H
#define XMRIG_ISPLITTER_H


#include <cstdint>


namespace xmrig {


class Upstreams
{
public:
    Upstreams() = default;


    inline Upstreams(uint64_t active, uint64_t sleep, uint64_t total) :
        active(active),
        sleep(sleep),
        total(total),
        error(total - active - sleep)
    {}


    inline double ratio(uint64_t miners) const { return active > 0 ? (static_cast<double>(miners) / active) : 0.0; }


    inline Upstreams &operator+=(const Upstreams &other)
    {
        active += other.active;
        sleep  += other.sleep;
        total  += other.total;
        error  += other.error;

        return *this;
    }


    uint64_t active = 0;
    uint64_t sleep  = 0;
    uint64_t total  = 0;
    uint64_t error  = 0;
};


class ISplitter
{
public:
    virtual ~ISplitter() = default;

    virtual Upstreams upstreams() const      = 0;
    virtual void connect()                   = 0;
    virtual void gc()                        = 0;
    virtual void printConnections()          = 0;
    virtual void tick(uint64_t ticks)        = 0;

#   ifdef APP_DEVEL
    virtual void printState()                = 0;
#   endif
};


} /* namespace xmrig */


#endif // XMRIG_ISPLITTER_H
