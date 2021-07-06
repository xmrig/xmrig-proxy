/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "base/io/log/Log.h"
#include "proxy/Counters.h"
#include "proxy/Miner.h"
#include "proxy/splitters/extra_nonce/ExtraNonceStorage.h"


bool xmrig::ExtraNonceStorage::add(Miner *miner)
{
    m_miners[miner->id()] = miner;

    if (isActive()) {
        miner->setJob(m_job, m_extraNonce);
        ++m_extraNonce;
    }

    return true;
}


bool xmrig::ExtraNonceStorage::isValidJobId(const String &id) const
{
    if (m_job.id() == id) {
        return true;
    }

    if (m_prevJob.isValid() && m_prevJob.id() == id) {
        Counters::expired++;
        return true;
    }

    return false;
}


xmrig::Miner *xmrig::ExtraNonceStorage::miner(int64_t id)
{
    auto it = m_miners.find(id);
    if (it != m_miners.end()) {
        return it->second;
    }

    return nullptr;
}


void xmrig::ExtraNonceStorage::remove(const Miner *miner)
{
    auto it = m_miners.find(miner->id());
    if (it != m_miners.end()) {
        m_miners.erase(it);
    }
}


void xmrig::ExtraNonceStorage::reset()
{
}


void xmrig::ExtraNonceStorage::setJob(const Job &job)
{
    if (m_job.clientId() == job.clientId()) {
        m_prevJob = m_job;
    }
    else {
        m_prevJob.reset();
    }

    m_job = job;

    m_extraNonce = 0;

    for (const auto& m : m_miners) {
        m.second->setJob(m_job, m_extraNonce);
        ++m_extraNonce;
    }
}


#ifdef APP_DEVEL
void xmrig::ExtraNonceStorage::printState(size_t id)
{
    LOG_INFO("#%03u - \x1B[35m%03u\x1B[0m", id, m_miners.size());
}
#endif
