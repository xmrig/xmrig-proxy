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
#include "base/net/stratum/AlgoSwitch.h"
#include "3rdparty/rapidjson/document.h"
#include "base/io/log/Log.h"
#include "proxy/Miner.h"


#include <algorithm>
#include <cassert>
#include <iterator>


namespace xmrig {


static bool algoLess(const Algorithm &left, const Algorithm &right)
{
    return left.id() < right.id();
}


static Algorithms normalizeAlgos(Algorithms algos)
{
    algos.erase(std::remove_if(algos.begin(), algos.end(), [](const Algorithm &algo) { return !algo.isValid(); }), algos.end());
    std::sort(algos.begin(), algos.end(), algoLess);
    algos.erase(std::unique(algos.begin(), algos.end(), [](const Algorithm &left, const Algorithm &right) { return left.id() == right.id(); }), algos.end());

    return algos;
}


static bool appendDefault(Algorithms &algos, algo_perfs &perfs, Algorithm::Id id, float perf)
{
    const Algorithm algo(id);
    if (!algo.isValid() || perfs.count(id)) {
        return false;
    }

    algos.emplace_back(id);
    perfs.insert(algo_perf(id, perf));

    return true;
}


AlgoSwitch::AlgoSwitch()
{
    setDefaultAlgo(Algorithm(Algorithm::RX_0));
}


Algorithms AlgoSwitch::intersection(Algorithms left, Algorithms right) const
{
    left = normalizeAlgos(std::move(left));
    right = normalizeAlgos(std::move(right));

    Algorithms out;
    std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(out), algoLess);

    return out;
}


algo_perfs AlgoSwitch::intersection(const algo_perfs &left, const algo_perfs &right) const
{
    algo_perfs out;
    auto l = left.begin();
    auto r = right.begin();

    while (l != left.end() && r != right.end()) {
        if (l->first < r->first) {
            ++l;
        }
        else if (r->first < l->first) {
            ++r;
        }
        else {
            out.insert(algo_perf(l->first, l->second + r->second));
            ++l;
            ++r;
        }
    }

    return out;
}


AlgoSwitch::MinerAlgoPerfData AlgoSwitch::minerData(const Miner *miner) const
{
    if (miner->get_algos().empty() && miner->get_algo_perfs().empty()) {
        return { m_defaultAlgos, m_defaultAlgoPerfs };
    }

    return { miner->get_algos(), miner->get_algo_perfs() };
}


void AlgoSwitch::computeCommonMinerAlgoPerfs()
{
    m_algos.clear();
    m_algoPerfs.clear();

    for (const auto &minerAlgoPerf : m_minerAlgoPerfs) {
        const Algorithms &algos = minerAlgoPerf.second.first;
        const algo_perfs &perfs = minerAlgoPerf.second.second;

        m_algos = m_algos.empty() ? algos : intersection(m_algos, algos);
        m_algoPerfs = m_algoPerfs.empty() ? perfs : intersection(m_algoPerfs, perfs);
    }
}


void AlgoSwitch::setDefaultAlgo(const Algorithm &algorithm)
{
    m_defaultAlgos.clear();
    m_defaultAlgoPerfs.clear();

    if (algorithm.isValid()) {
        appendDefault(m_defaultAlgos, m_defaultAlgoPerfs, algorithm.id(), 1000.0F);
    }
    else {
        appendDefault(m_defaultAlgos, m_defaultAlgoPerfs, Algorithm::RX_0, 1000.0F);
        appendDefault(m_defaultAlgos, m_defaultAlgoPerfs, Algorithm::RX_V2, 1000.0F);
    }

    appendDefault(m_defaultAlgos, m_defaultAlgoPerfs, Algorithm::CN_HEAVY_XHV, 10.0F);
    appendDefault(m_defaultAlgos, m_defaultAlgoPerfs, Algorithm::CN_HALF, 1.0F);
}


rapidjson::Value AlgoSwitch::algosToJSON(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    rapidjson::Value algos(rapidjson::kArrayType);

    for (const auto &algo : m_algos.empty() ? m_defaultAlgos : m_algos) {
        algos.PushBack(algo.toJSON(), allocator);
    }

    return algos;
}


rapidjson::Value AlgoSwitch::algoPerfsToJSON(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();
    rapidjson::Value perfs(rapidjson::kObjectType);

    for (const auto &algoPerf : m_algoPerfs.empty() ? m_defaultAlgoPerfs : m_algoPerfs) {
        perfs.AddMember(rapidjson::StringRef(Algorithm(algoPerf.first).name()), algoPerf.second, allocator);
    }

    return perfs;
}


void AlgoSwitch::setSameThreshold(uint64_t percent)
{
    m_percent = percent;
}


bool AlgoSwitch::tryMiner(const Miner *miner, const int upstreamCount) const
{
    if (m_minerAlgoPerfs.empty()) {
        return true;
    }

    const MinerAlgoPerfData data = minerData(miner);
    const Algorithms &algos = data.first;
    const algo_perfs &perfs = data.second;

    const Algorithms commonAlgos = intersection(m_algos, algos);
    if (commonAlgos.empty()) {
        return false;
    }

    for (const Algorithm &algo : commonAlgos) {
        const auto group = m_algoPerfs.find(algo.id());
        const auto candidate = perfs.find(algo.id());

        if (group == m_algoPerfs.end() || candidate == perfs.end()) {
            return false;
        }

        if (candidate->second == 0.0F) {
            if (group->second == 0.0F) {
                continue;
            }

            return false;
        }

        const float ratio = group->second / static_cast<float>(m_minerAlgoPerfs.size()) / candidate->second;
        const float threshold = static_cast<float>(m_percent + upstreamCount) / 100.0F;
        if (ratio > (1.0F + threshold) || ratio < (1.0F - threshold)) {
            return false;
        }
    }

    return true;
}


void AlgoSwitch::addMiner(const Miner *miner)
{
    const MinerAlgoPerfData data = minerData(miner);
    const Algorithms algos = m_minerAlgoPerfs.empty() ? data.first : intersection(m_algos, data.first);
    const algo_perfs perfs = m_minerAlgoPerfs.empty() ? data.second : intersection(m_algoPerfs, data.second);

    if ((!m_algos.empty() && algos.empty()) || (!m_algoPerfs.empty() && perfs.empty())) {
        LOG_WARN("[%s] ignoring miner for algo/algo-perf calculations because it would leave no common MoneroOcean algorithms", miner->ip());
        return;
    }

    m_algos = algos;
    m_algoPerfs = perfs;
    m_minerAlgoPerfs.insert({ miner->id(), data });
}


void AlgoSwitch::removeMiner(const Miner *miner)
{
    m_minerAlgoPerfs.erase(miner->id());
    computeCommonMinerAlgoPerfs();
}


} /* namespace xmrig */
/* MoneroOcean change: end */
