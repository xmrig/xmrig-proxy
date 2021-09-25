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

    virtual Upstreams upstreams() const             = 0;
    virtual void connect()                          = 0;
    virtual void gc()                               = 0;
    virtual void printConnections()                 = 0;
    virtual void tick(uint64_t ticks, uint64_t now) = 0;

#   ifdef APP_DEVEL
    virtual void printState()                       = 0;
#   endif
};


} // namespace xmrig


#endif // XMRIG_ISPLITTER_H
