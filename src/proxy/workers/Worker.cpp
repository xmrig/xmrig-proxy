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

#include <chrono>


#include "base/net/stratum/SubmitResult.h"
#include "proxy/workers/Worker.h"


xmrig::Worker::Worker() :
    m_id(0),
    m_hashrate(4),
    m_accepted(0),
    m_connections(0),
    m_hashes(0),
    m_invalid(0),
    m_lastHash(0),
    m_rejected(0)
{
}


xmrig::Worker::Worker(size_t id, const std::string &name, const std::string &ip) :
    m_id(id),
    m_ip(ip),
    m_name(name),
    m_hashrate(4),
    m_accepted(0),
    m_connections(1),
    m_hashes(0),
    m_invalid(0),
    m_lastHash(0),
    m_rejected(0)
{
}


void xmrig::Worker::add(uint64_t diff)
{
    m_accepted++;
    m_hashes += diff;

    m_hashrate.add(diff);

    using namespace std::chrono;
    m_lastHash = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


void xmrig::Worker::tick(uint64_t ticks)
{
    m_hashrate.tick();
}
