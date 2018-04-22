/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "common/log/Log.h"
#include "proxy/Addr.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Miner.h"
#include "proxy/Server.h"


Server::Server(const Addr &addr, bool nicehash) :
    m_nicehash(nicehash),
    m_version(0),
    m_port(addr.port()),
    m_ip(addr.ip())
{
    uv_tcp_init(uv_default_loop(), &m_server);
    m_server.data = this;

    uv_tcp_nodelay(&m_server, 1);

    if (addr.isIPv6() && uv_ip6_addr(m_ip.data(), m_port, &m_addr6) == 0) {
        m_version = 6;
        return;
    }

    if (uv_ip4_addr(m_ip.data(), m_port, &m_addr) == 0) {
        m_version = 4;
    }
}


bool Server::bind()
{
    if (!m_version) {
        return false;
    }

    const sockaddr *addr = m_version == 6 ? reinterpret_cast<const sockaddr*>(&m_addr6) : reinterpret_cast<const sockaddr*>(&m_addr);
    uv_tcp_bind(&m_server, addr, m_version == 6 ? UV_TCP_IPV6ONLY : 0);

    const int r = uv_listen(reinterpret_cast<uv_stream_t*>(&m_server), 511, Server::onConnection);
    if (r) {
        LOG_ERR("[%s:%u] listen error: \"%s\"", m_ip.data(), m_port, uv_strerror(r));
        return false;
    }

    return true;
}


void Server::create(uv_stream_t *server, int status)
{
    if (status < 0) {
        LOG_ERR("[%s:%u] new connection error: \"%s\"", m_ip.data(), m_port, uv_strerror(status));
        return;
    }

    Miner *miner = new Miner(m_nicehash, m_version == 6);
    if (!miner) {
        return;
    }

    if (!miner->accept(server)) {
        delete miner;
        return;
    }

    ConnectionEvent::start(miner, m_port);
}


void Server::onConnection(uv_stream_t *server, int status)
{
    static_cast<Server*>(server->data)->create(server, status);
}
