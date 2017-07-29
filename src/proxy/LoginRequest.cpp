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


#include "proxy/LoginRequest.h"


LoginRequest::LoginRequest(int64_t id, const char *login, const char *pass, const char *agent) :
    m_agent(agent),
    m_login(login),
    m_pass(pass),
    m_id(id)
{
    m_clientType = detectClient();
}


LoginRequest::ClientTypes LoginRequest::detectClient() const
{
    if (!m_agent || strlen(m_agent) < 32) {
        return OtherClient;
    }

    if (memcmp(m_agent, "XMRig/2.0.", 10) == 0) {
        return XMRig20Client;
    }

    if (memcmp(m_agent, "XMRig/", 6) == 0) {
        return XMRigClient;
    }

    if (memcmp(m_agent, "xmrig-proxy/", 13) == 0) {
        return XMRigProxyClient;
    }

    return OtherClient;
}
