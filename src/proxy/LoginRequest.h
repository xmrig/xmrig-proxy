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

#ifndef __LOGINREQUEST_H__
#define __LOGINREQUEST_H__


#include <stdint.h>
#include <string.h>


class LoginRequest
{
public:
    inline LoginRequest() :
        m_agent(nullptr),
        m_login(nullptr),
        m_pass(nullptr),
        m_id(0)
    {}

    inline LoginRequest(int64_t id, const char *login, const char *pass, const char *agent, const char *rigId) :
        m_agent(agent),
        m_login(login),
        m_pass(pass),
        m_rigId(rigId),
        m_id(id)
    {}

    inline const char *agent() const      { return m_agent; }
    inline const char *login() const      { return m_login; }
    inline const char *pass() const       { return m_pass; }
    inline const char *rigId() const      { return m_rigId && strlen(m_rigId) > 0 ? m_rigId : m_login; }
    inline int64_t id() const             { return m_id; }

private:
    const char *m_agent;
    const char *m_login;
    const char *m_pass;
    const char *m_rigId;
    const int64_t m_id;
};

#endif /* __LOGINREQUEST_H__ */
