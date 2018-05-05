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

#include <chrono>


#include "common/net/SubmitResult.h"
#include "proxy/workers/Worker.h"


Worker::Worker() :
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


Worker::Worker(size_t id, const std::string &name, const std::string &ip) :
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


void Worker::add(const SubmitResult &result)
{
    m_accepted++;
    m_hashes += result.diff;

    m_hashrate.add(result.diff);

    using namespace std::chrono;
    m_lastHash = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


void Worker::tick(uint64_t ticks)
{
    m_hashrate.tick();
}
