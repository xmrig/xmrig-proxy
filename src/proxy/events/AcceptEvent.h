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

#ifndef __ACCEPTEVENT_H__
#define __ACCEPTEVENT_H__


#include "proxy/events/MinerEvent.h"
#include "proxy/Error.h"


class SubmitResult;


class AcceptEvent : public MinerEvent
{
public:
    static inline bool start(Miner *miner, const SubmitResult &result, const char *error = nullptr)
    {
        return exec(new (m_buf) AcceptEvent(miner, result, error));
    }


    const SubmitResult &result;


    inline bool isRejected() const override { return m_error != nullptr; }
    inline const char *error() const        { return m_error; }


protected:
    inline AcceptEvent(Miner *miner, const SubmitResult &result, const char *error)
        : MinerEvent(AcceptType, miner),
          result(result),
          m_error(error)
    {}

private:
    const char *m_error;
};

#endif /* __ACCEPTEVENT_H__ */
