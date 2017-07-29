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


#include <string.h>
#include <uv.h>


#include "proxy/Hashrate.h"



Hashrate::Hashrate() :
    accepted(0),
    shares(0),
    m_tickShares(0),
    m_datetime(0)
{
    memset(rejected, 0, sizeof(rejected));
}


double Hashrate::calc(size_t seconds)
{
    const size_t ticks = seconds / kTickTime;
    const size_t size  = m_data.size();

    uint64_t count = 0;
    for (size_t i = size < ticks ? 0 : (size - ticks); i < size; ++i) {
        count += m_data[i];
    }

    return (double) count / (ticks * kTickTime * 1000);
}


void Hashrate::add(uint32_t diff)
{
    shares += diff;
    m_tickShares += diff;

    m_datetime = uv_now(uv_default_loop());
}


void Hashrate::tick()
{
    m_data.push_back(m_tickShares);
    m_tickShares = 0;
}
