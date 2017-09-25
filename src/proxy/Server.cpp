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


#include "log/Log.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Miner.h"
#include "proxy/Server.h"


Server::Server(const char *ip, uint16_t port) :
    m_ip(ip),
    m_port(port)
{
    uv_tcp_init(uv_default_loop(), &m_server);
    m_server.data = this;

    uv_ip4_addr(ip, port, &m_addr);
    uv_tcp_nodelay(&m_server, 1);
}


bool Server::bind()
{
    uv_tcp_bind(&m_server, reinterpret_cast<const sockaddr*>(&m_addr), 0);

    const int r = uv_listen(reinterpret_cast<uv_stream_t*>(&m_server), 511, Server::onConnection);
    if (r) {
        LOG_ERR("[%s:%u] listen error: \"%s\"", m_ip, m_port, uv_strerror(r));
        return false;
    }

    return true;
}


void Server::onConnection(uv_stream_t *server, int status)
{
    auto instance = static_cast<Server*>(server->data);

    if (status < 0) {
        LOG_ERR("[%s:%u] new connection error: \"%s\"", instance->m_ip, instance->m_port, uv_strerror(status));
        return;
    }

    Miner *miner = new Miner();
    if (!miner) {
        LOG_ERR("NEW FAILED");
        return;
    }

    if (!miner->accept(server)) {
        delete miner;
        return;
    }

    ConnectionEvent::start(miner, instance->m_port);
}
