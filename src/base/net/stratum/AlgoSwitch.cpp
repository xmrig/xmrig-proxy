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

#include <cassert>
#include "base/net/stratum/AlgoSwitch.h"
#include <algorithm>
#include "proxy/Miner.h"
#include "base/io/log/Log.h"

namespace xmrig {

Algorithms AlgoSwitch::intersection(Algorithms a1, Algorithms a2) const {
  Algorithms r;
  std::sort(a1.begin(), a1.end());
  std::sort(a2.begin(), a2.end());
  std::set_intersection(a1.begin(), a1.end(), a2.begin(), a2.end(), std::back_inserter(r));
  return r;
}

algo_perfs AlgoSwitch::intersection(const algo_perfs& a1, const algo_perfs& a2) const {
  algo_perfs r;
  algo_perfs::const_iterator i1 = a1.begin();
  algo_perfs::const_iterator i2 = a2.begin();
  while (i1 != a1.end() && i2 != a2.end()) {
    if (i1->first < i2->first) ++ i1;
    else if (i2->first < i1->first) ++ i2;
    else {
      r.insert(algo_perf(i1->first, i1->second + i2->second));
      ++ i1;
      ++ i2;
    }
  }
  return r;
}

void AlgoSwitch::compute_common_miner_algo_perfs() {
  m_algos.clear();
  m_algo_perfs.clear();
  for (const auto& miner_algo_perf: m_miner_algo_perfs) {
    const Algorithms& algos = miner_algo_perf.second.first;
    const algo_perfs& algo_perfs = miner_algo_perf.second.second;
    m_algos      = m_algos.empty()      ? algos      : intersection(m_algos, algos);
    m_algo_perfs = m_algo_perfs.empty() ? algo_perfs : intersection(m_algo_perfs, algo_perfs);
  }
}

void AlgoSwitch::setDefaultAlgoSwitchAlgo(const Algorithm& algo) {
  m_default_algos.clear();
  m_default_algo_perfs.clear();
  if (algo.isValid()) {
    m_default_algos.push_back(algo.id());
    m_default_algo_perfs.insert(algo_perf(algo.id(), 1000));
  } else {
    m_default_algos.push_back(Algorithm::RX_0);
    m_default_algo_perfs.insert(algo_perf(Algorithm::RX_0, 1000));
  }
  m_default_algos.push_back(Algorithm::CN_HEAVY_XHV);
  m_default_algo_perfs.insert(algo_perf(Algorithm::CN_HEAVY_XHV, 10));
  m_default_algos.push_back(Algorithm::CN_HALF);
  m_default_algo_perfs.insert(algo_perf(Algorithm::CN_HALF, 1));
}

rapidjson::Value AlgoSwitch::algos_toJSON(rapidjson::Document& doc) const {
  auto &allocator = doc.GetAllocator();
  rapidjson::Value algos(rapidjson::kArrayType);
  for (const auto& algo: m_algos.empty() ? m_default_algos : m_algos) {
    algos.PushBack(algo.toJSON(), allocator);
  }
  return algos;
}

rapidjson::Value AlgoSwitch::algo_perfs_toJSON(rapidjson::Document& doc) const {
  auto &allocator = doc.GetAllocator();
  rapidjson::Value algo_perfs(rapidjson::kObjectType);
  for (const auto& algo_perf: m_algo_perfs.empty() ? m_default_algo_perfs : m_algo_perfs) {
    algo_perfs.AddMember(rapidjson::StringRef(Algorithm(algo_perf.first).name()), algo_perf.second, allocator);
  }
  return algo_perfs;
}

void AlgoSwitch::set_algo_perf_same_threshold(uint64_t percent) {
  m_percent = percent;
}

bool AlgoSwitch::try_miner(const Miner* miner, const int upstream_count) const {
  if (m_miner_algo_perfs.empty()) return true;
  // make sure algos are the same, if not return false
  if (m_algos.size() != miner->get_algos().size() || intersection(m_algos, miner->get_algos()).size() < m_algos.size()) return false;
  // make sure algo_perfs has the same keys, if not return false
  if (m_algo_perfs.size() != miner->get_algo_perfs().size() || intersection(m_algo_perfs, miner->get_algo_perfs()).size() < m_algo_perfs.size()) return false;
  // if any hashrate ratio of more than 20% then we do not place miners in the same group
  algo_perfs::const_iterator i1 = m_algo_perfs.begin();
  algo_perfs::const_iterator i2 = miner->get_algo_perfs().begin();
  for (; i1 != m_algo_perfs.end(); ++ i1, ++ i2) {
    assert(i1->first == i2->first);
    if (i2->second == 0.0f) {
      if (i1->second == 0.0f) continue;
      return false;
    }
    const float ratio = i1->second / m_miner_algo_perfs.size() / i2->second;
    if (ratio > (1.0f + (float)(m_percent + upstream_count) / 100.0f) || ratio < (1.0f - (float)(m_percent + upstream_count) / 100.0f)) return false;
  }
  return true;
}

void AlgoSwitch::add_miner(const Miner* miner) {
  const Algorithms algos      = m_miner_algo_perfs.empty() ? miner->get_algos()      : intersection(m_algos, miner->get_algos());
  const algo_perfs algo_perfs = m_miner_algo_perfs.empty() ? miner->get_algo_perfs() : intersection(m_algo_perfs, miner->get_algo_perfs());
  if ((!m_algos.empty() && algos.empty()) || (!m_algo_perfs.empty() && algo_perfs.empty())) {
    LOG_WARN("[%s] ignoring miner for algo/algo-perf calcs since it makes them empty", miner->ip());
  } else {
    m_algos      = algos;
    m_algo_perfs = algo_perfs;
    m_miner_algo_perfs.insert(miner_algo_perf(miner->id(), miner_algo_perf_data(miner->get_algos(), miner->get_algo_perfs())));
  }
}

void AlgoSwitch::del_miner(const Miner* miner) {
  m_miner_algo_perfs.erase(miner->id());
  compute_common_miner_algo_perfs();
}


} /* namespace xmrig */
