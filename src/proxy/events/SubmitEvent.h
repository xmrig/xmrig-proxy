/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_SUBMITEVENT_H
#define XMRIG_SUBMITEVENT_H


#include "net/JobResult.h"
#include "proxy/Error.h"
#include "proxy/events/MinerEvent.h"


namespace xmrig {


class SubmitEvent : public MinerEvent
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(SubmitEvent)

    inline SubmitEvent(Miner *miner, int64_t id, const char *jobId, const char *nonce, const char *result, const Algorithm &algorithm, const char* sig, const char* sig_data, int64_t extra_nonce)
        : MinerEvent(miner),
          request(id, jobId, nonce, result, algorithm, sig, sig_data, extra_nonce),
          m_error(Error::NoError)
    {}

    bool expired = false;
    JobResult request;

    inline bool isRejected() const override { return m_error != Error::NoError; }
    inline const char *message() const      { return Error::toString(m_error); }
    inline Error::Code error() const        { return m_error; }
    inline void reject(Error::Code error)   { m_error  = error; }

protected:    
    uint32_t type() const override          { return SUBMIT_EVENT; }

private:
    Error::Code m_error;
};


} // namespace xmrig


#endif // XMRIG_SUBMITEVENT_H
