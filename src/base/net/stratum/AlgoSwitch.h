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

#pragma once

#include "3rdparty/rapidjson/document.h"
#include "base/crypto/Algorithm.h"

namespace xmrig {

class Miner;

class AlgoSwitch {
  typedef std::pair<Algorithms, algo_perfs> miner_algo_perf_data;
  typedef std::pair<int64_t, miner_algo_perf_data> miner_algo_perf;
  typedef std::map<int64_t, miner_algo_perf_data> miner_algo_perfs;
  uint64_t m_percent = 20;
  miner_algo_perfs m_miner_algo_perfs;
  Algorithms m_algos, m_default_algos;
  algo_perfs m_algo_perfs, m_default_algo_perfs;

  Algorithms intersection(Algorithms, Algorithms) const;
  algo_perfs intersection(const algo_perfs&, const algo_perfs&) const;

  void compute_common_miner_algo_perfs();

  public:
    AlgoSwitch() { setDefaultAlgoSwitchAlgo(Algorithm(Algorithm::RX_0)); }
    void setDefaultAlgoSwitchAlgo(const Algorithm&);
    rapidjson::Value algos_toJSON(rapidjson::Document&) const;
    rapidjson::Value algo_perfs_toJSON(rapidjson::Document&) const;
    void set_algo_perf_same_threshold(uint64_t);
    bool try_miner(const Miner*) const;
    void add_miner(const Miner*);
    void del_miner(const Miner*);
};

} /* namespace xmrig */
