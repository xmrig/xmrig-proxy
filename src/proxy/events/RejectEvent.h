/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 *
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

#ifndef __REJECTEVENT_H__
#define __REJECTEVENT_H__


#include <stdint.h>


#include "proxy/events/MinerEvent.h"
#include "proxy/Error.h"


class RejectEvent : public MinerEvent
{
public:
    static inline RejectEvent *create(Miner *miner, int64_t id, Error::Type error)
    {
        return new (m_buf) RejectEvent(miner, id, error);
    }


    static inline RejectEvent *create(Miner *miner, int64_t id, const char *message)
    {
        return new (m_buf) RejectEvent(miner, id, message);
    }


    inline bool isLocal() const        { return m_local; }
    inline const char *message() const { return m_message; }
    inline int64_t id() const          { return m_id; }


protected:
    inline RejectEvent(Miner *miner, int64_t id, Error::Type error)
        : MinerEvent(RejectType, miner),
          m_local(true),
          m_message(Error::toString(error)),
          m_id(id)
    {}


    inline RejectEvent(Miner *miner, int64_t id, const char *message)
        : MinerEvent(RejectType, miner),
          m_local(true),
          m_message(message),
          m_id(id)
    {}


    inline bool isRejected() const override { return true; }


private:
    const bool m_local;
    const char *m_message;
    int64_t m_id;
};


#endif /* __REJECTEVENT_H__ */
