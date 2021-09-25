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

#include "proxy/Server.h"
#include "base/io/log/Log.h"
#include "base/kernel/Events.h"
#include "base/kernel/Process.h"
#include "base/tools/Handle.h"
#include "proxy/BindHost.h"
#include "proxy/events/ConnectionEvent.h"
#include "proxy/Miner.h"


xmrig::Server::Server(const BindHost &host, const TlsContext *ctx) :
    m_strictTls(host.isTLS()),
    m_host(host.host()),
    m_ctx(ctx),
    m_port(host.port())
{
    m_server = new uv_tcp_t;

    uv_tcp_init(uv_default_loop(), m_server);
    m_server->data = this;

    uv_tcp_nodelay(m_server, 1);

    if (host.isIPv6() && uv_ip6_addr(m_host.data(), m_port, reinterpret_cast<sockaddr_in6 *>(&m_addr)) == 0) {
        m_version = 6;
        return;
    }

    if (uv_ip4_addr(m_host.data(), m_port, reinterpret_cast<sockaddr_in *>(&m_addr)) == 0) {
        m_version = 4;
    }
}


xmrig::Server::~Server()
{
    Handle::close(m_server);
}


bool xmrig::Server::bind()
{
    if (!m_version) {
        return false;
    }

    uv_tcp_bind(m_server, reinterpret_cast<const sockaddr*>(&m_addr), m_version == 6 ? UV_TCP_IPV6ONLY : 0);

    const int r = uv_listen(reinterpret_cast<uv_stream_t*>(m_server), 511, Server::onConnection);
    if (r) {
        LOG_ERR("[%s:%u] listen error: \"%s\"", m_host.data(), m_port, uv_strerror(r));
        return false;
    }

    return true;
}


void xmrig::Server::create(uv_stream_t *server, int status)
{
    if (status < 0) {
        LOG_ERR("[%s:%u] new connection error: \"%s\"", m_host.data(), m_port, uv_strerror(status));
        return;
    }

    auto miner = new Miner(m_ctx, m_port, m_strictTls);
    if (!miner) {
        return;
    }

    if (!miner->accept(server)) {
        delete miner;
        return;
    }

    Process::events().send<ConnectionEvent>(miner, m_port);
}


void xmrig::Server::onConnection(uv_stream_t *server, int status)
{
    static_cast<Server*>(server->data)->create(server, status);
}
