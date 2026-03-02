/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_ACCEPTEVENT_H
#define XMRIG_ACCEPTEVENT_H


#include "base/net/stratum/SubmitResult.h"
#include "proxy/Miner.h"
#include "proxy/events/MinerEvent.h"
#include "proxy/Error.h"


namespace xmrig {


class AcceptEvent : public MinerEvent
{
public:
    static inline bool start(size_t mapperId, Miner *miner, const SubmitResult &result, bool donate, bool customDiff, const char *error = nullptr)
    {
        return exec(new (m_buf) AcceptEvent(mapperId, miner, result, donate, customDiff, error));
    }


    const SubmitResult &result;


    inline bool isCustomDiff() const        { return m_customDiff; }
    inline bool isDonate() const            { return m_donate; }
    inline bool isRejected() const override { return m_error != nullptr; }
    inline const char *error() const        { return m_error; }
    inline size_t mapperId() const          { return m_mapperId; }
    inline uint64_t statsDiff() const       { return (miner() && miner()->customDiff() ? std::min(miner()->customDiff(), result.diff) : result.diff); }


protected:
    inline AcceptEvent(size_t mapperId, Miner *miner, const SubmitResult &result, bool donate, bool customDiff, const char *error)
        : MinerEvent(AcceptType, miner),
          result(result),
          m_customDiff(customDiff),
          m_donate(donate),
          m_error(error),
          m_mapperId(mapperId)
    {}


private:
    bool m_customDiff;
    bool m_donate;
    const char *m_error;
    size_t m_mapperId;
};


} /* namespace xmrig */


#endif /* XMRIG_ACCEPTEVENT_H */
