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

#ifndef XMRIG_NONCESTORAGE_H
#define XMRIG_NONCESTORAGE_H


#include <map>
#include <vector>


#include "common/net/Job.h"


class Miner;


class NonceStorage
{
public:
    NonceStorage();
    ~NonceStorage();

    bool add(Miner *miner);
    bool isUsed() const;
    bool isValidJobId(const xmrig::Id &id) const;
    Miner *miner(int64_t id);
    void remove(const Miner *miner);
    void reset();
    void setJob(const Job &job);

    inline bool isActive() const       { return m_active; }
    inline const Job &job() const      { return m_job; }
    inline void setActive(bool active) { m_active = active; }

#   ifdef APP_DEVEL
    void printState(size_t id);
#   endif

private:
    int nextIndex(int start) const;

    bool m_active;
    Job m_job;
    Job m_prevJob;
    std::map<int64_t, Miner*> m_miners;
    std::vector<int64_t> m_used;
    uint8_t m_index;
};


#endif /* XMRIG_NONCESTORAGE_H */
