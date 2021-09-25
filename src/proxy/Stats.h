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

#ifndef XMRIG_STATS_H
#define XMRIG_STATS_H


#include "base/kernel/EventListener.h"


namespace xmrig {


class ConfigEvent;
class ISplitter;
class StatsData;


class Stats : public EventListener
{
public:
    XMRIG_DISABLE_COPY_MOVE(Stats)

    Stats(const ConfigEvent *event);
    ~Stats() override = default;

    const StatsData &data() const;
    double hashrate(int seconds) const;
    void tick(uint64_t ticks, const ISplitter *splitter);

protected:
    void onEvent(uint32_t type, IEvent *event) override;

private:
    XMRIG_DECL_PRIVATE()
};


} // namespace xmrig


#endif // XMRIG_STATS_H
