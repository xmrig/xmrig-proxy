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

#ifndef XMRIG_LOGINEVENT_H
#define XMRIG_LOGINEVENT_H


#include "common/crypto/Algorithm.h"
#include "proxy/events/MinerEvent.h"


class LoginEvent : public MinerEvent
{
public:
    static inline LoginEvent *create(Miner *miner, int64_t id, const char *login, const char *pass, const char *agent, const char *rigId, const xmrig::Algorithms &algorithms)
    {
        return new (m_buf) LoginEvent(miner, id, login, pass, agent, rigId, algorithms);
    }


    const int64_t loginId;
    const xmrig::Algorithms &algorithms;


protected:
    inline LoginEvent(Miner *miner, int64_t id, const char *login, const char *pass, const char *agent, const char *rigId, const xmrig::Algorithms &algorithms)
        : MinerEvent(LoginType, miner),
          loginId(id),
          algorithms(algorithms)
    {}
};

#endif /* XMRIG_LOGINEVENT_H */
