/* XMRig
 * Copyright 2020 MoneroOcean <https://github.com/MoneroOcean>, <support@moneroocean.stream>
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

/* MoneroOcean change: begin MoneroOcean pools choose work from miner algo/algo-perf capability sets, so the proxy keeps a normalized group-wide set per upstream client. */
#ifndef XMRIG_ALGOSWITCH_H
#define XMRIG_ALGOSWITCH_H


#include "3rdparty/rapidjson/fwd.h"
#include "base/crypto/Algorithm.h"


namespace xmrig {


class Miner;


class AlgoSwitch
{
public:
    AlgoSwitch();

    bool tryMiner(const Miner *miner, int upstreamCount) const;
    rapidjson::Value algoPerfsToJSON(rapidjson::Document &doc) const;
    rapidjson::Value algosToJSON(rapidjson::Document &doc) const;
    void addMiner(const Miner *miner);
    void removeMiner(const Miner *miner);
    void setDefaultAlgo(const Algorithm &algorithm);
    void setSameThreshold(uint64_t percent);

private:
    using MinerAlgoPerfData = std::pair<Algorithms, algo_perfs>;
    using MinerAlgoPerfs = std::map<int64_t, MinerAlgoPerfData>;

    Algorithms intersection(Algorithms left, Algorithms right) const;
    MinerAlgoPerfData minerData(const Miner *miner) const;
    algo_perfs intersection(const algo_perfs &left, const algo_perfs &right) const;
    void computeCommonMinerAlgoPerfs();

    Algorithms m_algos;
    Algorithms m_defaultAlgos;
    MinerAlgoPerfs m_minerAlgoPerfs;
    algo_perfs m_algoPerfs;
    algo_perfs m_defaultAlgoPerfs;
    uint64_t m_percent = 20;
};


} /* namespace xmrig */


#endif /* XMRIG_ALGOSWITCH_H */
/* MoneroOcean change: end */
