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

#ifndef XMRIG_LOGINEVENT_H
#define XMRIG_LOGINEVENT_H


#include "base/crypto/Algorithm.h"
#include "base/tools/String.h"
#include "proxy/events/MinerEvent.h"


namespace xmrig {


class LoginEvent : public MinerEvent
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(LoginEvent)

    inline LoginEvent(Miner *miner, int64_t id, const Algorithms &algorithms, const rapidjson::Value &params)
        : MinerEvent(miner),
          algorithms(algorithms),
          loginId(id),
          params(params)
    {}

    ~LoginEvent() = default;

    const Algorithms &algorithms;
    const int64_t loginId;
    const rapidjson::Value &params;
    int error = -1;
    String flow;

protected:
    uint32_t type() const override  { return LOGIN_EVENT; }
};


} // namespace xmrig


#endif // XMRIG_LOGINEVENT_H
