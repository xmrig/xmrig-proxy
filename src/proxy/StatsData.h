/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef __STATSDATA_H__
#define __STATSDATA_H__


#include <algorithm>
#include <array>
#include <uv.h>
#include <vector>


#include "interfaces/ISplitter.h"


class StatsData
{
public:
    inline StatsData() :
        accepted(0),
        connections(0),
        donateHashes(0),
        expired(0),
        hashes(0),
        invalid(0),
        maxMiners(0),
        miners(0),
        rejected(0),
        startTime(0)
    {
    }


    inline uint32_t avgTime() const
    {
        if (latency.empty()) {
            return 0;
        }

        return static_cast<uint32_t>(uptime() / latency.size());
    }


    inline uint32_t avgLatency() const
    {
        const size_t calls = latency.size();
        if (calls == 0) {
            return 0;
        }

        auto v = latency;
        std::nth_element(v.begin(), v.begin() + calls / 2, v.end());

        return v[calls / 2];
    }


    inline int uptime() const
    {
        if (startTime == 0) {
            return 0;
        }

        return (uv_now(uv_default_loop()) - startTime) / 1000;
    }


    double hashrate[6] { 0.0 };
    std::array<uint64_t, 10> topDiff { { } };
    std::vector<uint16_t> latency;
    uint64_t accepted;
    uint64_t connections;
    uint64_t donateHashes;
    uint64_t expired;
    uint64_t hashes;
    uint64_t invalid;
    uint64_t maxMiners;
    uint64_t miners;
    uint64_t rejected;
    uint64_t startTime;
    Upstreams upstreams;
};


#endif /* __STATS_H__ */
