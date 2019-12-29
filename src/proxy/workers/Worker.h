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

#ifndef XMRIG_WORKER_H
#define XMRIG_WORKER_H


#include <string>


#include "proxy/TickingCounter.h"


namespace xmrig {


class Worker
{
public:
    Worker();
    Worker(size_t id, const std::string &name, const std::string &ip);

    void add(uint64_t diff);
    void tick(uint64_t ticks);

    inline const char *ip() const             { return m_ip.c_str(); }
    inline const char *name() const           { return m_name.c_str(); }
    inline double hashrate(int seconds) const { return m_hashrate.calc(seconds); }
    inline size_t id() const                  { return m_id; }
    inline uint64_t accepted() const          { return m_accepted; }
    inline uint64_t connections() const       { return m_connections; }
    inline uint64_t hashes() const            { return m_hashes; }
    inline uint64_t invalid() const           { return m_invalid; }
    inline uint64_t lastHash() const          { return m_lastHash; }
    inline uint64_t rejected() const          { return m_rejected; }
    inline void add(const char *ip)           { m_ip = ip; m_connections++; }
    inline void reject(bool invalid)          { invalid ? m_invalid++ : m_rejected++; }
    inline void remove()                      { m_connections--; }

private:
    size_t m_id;
    std::string m_ip;
    std::string m_name;
    TickingCounter<uint32_t> m_hashrate;
    uint64_t m_accepted;
    uint64_t m_connections;
    uint64_t m_hashes;
    uint64_t m_invalid;
    uint64_t m_lastHash;
    uint64_t m_rejected;
};


} /* namespace xmrig */


#endif /* XMRIG_WORKER_H */
