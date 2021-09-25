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
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(AcceptEvent)

    inline AcceptEvent(size_t mapperId, Miner *miner, const SubmitResult &result, bool donate, bool customDiff, const char *error = nullptr)
        : MinerEvent(miner),
          result(result),
          m_customDiff(customDiff),
          m_donate(donate),
          m_error(error),
          m_mapperId(mapperId)
    {}

    ~AcceptEvent() override = default;

    const SubmitResult &result;

    inline bool isCustomDiff() const            { return m_customDiff; }
    inline bool isCustomDiffStats() const       { return m_customDiffStats; }
    inline bool isDonate() const                { return m_donate; }
    inline bool isRejected() const override     { return m_error != nullptr; }
    inline const char *error() const            { return m_error; }
    inline size_t mapperId() const              { return m_mapperId; }
    inline uint64_t statsDiff() const           { return (m_customDiffStats && miner() && miner()->customDiff() ? std::min(miner()->customDiff(), result.diff) : result.diff); }
    inline void setCustomDiffStats(bool enable) { m_customDiffStats = enable; }

//    inline uint64_t statsDiff() const       { return (miner() && miner()->customDiff() ? std::min(miner()->customDiff(), result.diff) : result.diff); }

protected:
    uint32_t type() const override              { return ACCEPT_EVENT; }

private:
    bool m_customDiffStats = 0;
    const bool m_customDiff;
    const bool m_donate;
    const char *m_error;
    const size_t m_mapperId;
};


} // namespace xmrig


#endif // XMRIG_ACCEPTEVENT_H
