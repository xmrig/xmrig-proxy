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

#include <inttypes.h>


#include "common/log/Log.h"
#include "proxy/Counters.h"
#include "proxy/Miner.h"
#include "proxy/splitters/nicehash/NonceStorage.h"


NonceStorage::NonceStorage() :
    m_active(false),
    m_used(256, 0),
    m_index(rand() % 256)
{
}


NonceStorage::~NonceStorage()
{
}


bool NonceStorage::add(Miner *miner)
{
    const int index = nextIndex(0);
    if (index == -1) {
        return false;
    }

    miner->setFixedByte(index);

    m_index = index;
    m_used[index] = miner->id();
    m_miners[miner->id()] = miner;

    if (isActive()) {
        miner->setJob(m_job);
    }

    return true;
}


bool NonceStorage::isUsed() const
{
    for (size_t i = 0; i < 256; ++i) {
     if (m_used[i] > 0) {
         return true;
     }
    }

    return false;
}


bool NonceStorage::isValidJobId(const xmrig::Id &id) const
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


Miner *NonceStorage::miner(int64_t id)
{
    if (m_miners.count(id) == 0) {
        return nullptr;
    }

    return m_miners.at(id);
}


void NonceStorage::remove(const Miner *miner)
{
    m_used[miner->fixedByte()] = -miner->id();

    auto it = m_miners.find(miner->id());
    if (it != m_miners.end()) {
        m_miners.erase(it);
    }
}


void NonceStorage::reset()
{
    std::fill(m_used.begin(), m_used.end(), 0);
}


void NonceStorage::setJob(const Job &job)
{
    for (size_t i = 0; i < 256; ++i) {
        if (m_used[i] < 0) {
            m_used[i] = 0;
        }
    }

    if (m_job.clientId() == job.clientId()) {
        m_prevJob = m_job;
    }
    else {
        m_prevJob.reset();
    }

    m_job = job;

    for (size_t i = 0; i < 256; ++i) {
        const int64_t index = m_used[i];
        if (index == 0) {
            continue;
        }

        Miner *miner = this->miner(index);
        if (miner) {
            miner->setJob(m_job);
        }
    }
}


#ifdef APP_DEVEL
void NonceStorage::printState(size_t id)
{    int available = 0;
     int dead      = 0;

     for (const int64_t v : m_used) {
         if (v == 0) {
             available++;
         }
         else if (v < 0) {
             dead++;
         }
     }

     int miners = 256 - available - dead;

     LOG_INFO("#%03u - \x1B[32m%03d \x1B[33m%03d \x1B[35m%03d\x1B[0m - 0x%02hhX, % 5.1f%%",
              id, available, dead, miners, m_index, (double) miners / 256 * 100.0);

}
#endif


int NonceStorage::nextIndex(int start) const
{
    for (size_t i = m_index; i < m_used.size(); ++i) {
        if (m_used[i] == 0) {
            return (int) i;
        }
    }

    for (size_t i = start; i < m_index; ++i) {
        if (m_used[i] == 0) {
            return (int) i;
        }
    }

    return -1;
}
