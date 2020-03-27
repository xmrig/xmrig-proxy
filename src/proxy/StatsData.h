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

#ifndef XMRIG_STATSDATA_H
#define XMRIG_STATSDATA_H


#include <algorithm>
#include <array>
#include <vector>


#include "base/tools/Chrono.h"
#include "proxy/interfaces/ISplitter.h"


namespace xmrig {


class StatsData
{
public:
    inline StatsData() : startTime(Chrono::currentMSecsSinceEpoch()) {}


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


    inline double ratio() const    { return upstreams.ratio(miners); }
    inline uint64_t uptime() const { return (Chrono::currentMSecsSinceEpoch() - startTime) / 1000; }


    inline StatsData &operator+=(const StatsData &other)
    {
        upstreams    += other.upstreams;
        accepted     += other.accepted;
        connections  += other.connections;
        donateHashes += other.donateHashes;
        expired      += other.expired;
        hashes       += other.hashes;
        invalid      += other.invalid;
        rejected     += other.rejected;

        for (size_t i = 0; i < 6; ++i) {
            hashrate[i] += other.hashrate[i];
        }

        return *this;
    }


    double hashrate[6] { 0.0 };
    std::array<uint64_t, 10> topDiff { { } };
    std::vector<uint16_t> latency;
    uint64_t accepted       = 0;
    uint64_t connections    = 0;
    uint64_t donateHashes   = 0;
    uint64_t expired        = 0;
    uint64_t hashes         = 0;
    uint64_t invalid        = 0;
    uint64_t maxMiners      = 0;
    uint64_t miners         = 0;
    uint64_t rejected       = 0;
    uint64_t startTime      = 0;
    Upstreams upstreams;
};


} /* namespace xmrig */


#endif /* XMRIG_STATSDATA_H */
