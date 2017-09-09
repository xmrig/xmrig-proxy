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

#ifndef __COUNTERS_H__
#define __COUNTERS_H__


#include <uv.h>


#include "proxy/Hashrate.h"


class Counters
{
public:
    enum Stores {
        Primary,
        Secondary
    };

    enum CounterTypes {
        Upstream
    };

    class Tick
    {
    public:
        inline Tick() : added(0), removed(0), accepted(0) {}

        uint32_t added;
        uint32_t removed;
        uint64_t accepted;

        inline void reset() {
            added    = 0;
            removed  = 0;
            accepted = 0;
        }
    };


    inline static uint64_t accepted()    { return m_hashrate[0].accepted + m_hashrate[1].accepted; }
    inline static uint64_t rejected()    { return m_hashrate[0].rejected[0] + m_hashrate[1].rejected[0]; }
    inline static uint64_t rejected2()   { return m_hashrate[0].rejected[1] + m_hashrate[1].rejected[1];; }
    inline static uint64_t upstreams()   { return m_counters[Upstream]; }

    static double hashrate(size_t seconds);
    static void accept(Stores store, size_t id, uint32_t diff, uint64_t ms, bool verbose);
    static void add(CounterTypes type);
    static void reject(Stores store, const char *ip, const char *message);
    static void reject(Stores store, size_t id, uint32_t diff, uint64_t ms, const char *error);
    static void remove(CounterTypes type);
    static void start();

    static Tick tick;

    static inline void reset()
    {
        tick.reset();
    }

private:
    static void onTick(uv_timer_t *handle);

    static Hashrate m_hashrate[2];
    static uint64_t m_counters[3];
    static uint64_t m_minersMax;
    static uv_timer_t m_timer;
};

#endif /* __COUNTERS_H__ */
