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

#ifndef __ADDR_H__
#define __ADDR_H__


#include <stdint.h>
#include <string.h>
#include <stdlib.h>


class Addr
{
public:
    constexpr static uint16_t kDefaultPort = 3333;


    inline Addr() :
        m_host(nullptr),
        m_port(kDefaultPort)
    {}


    inline Addr(const char *addr) :
        m_host(nullptr),
        m_port(kDefaultPort)
    {
        if (!addr) {
            return;
        }

        const char *port = strchr(addr, ':');
        if (!port) {
            m_host = strdup(addr);
            return;
        }

        const size_t size = port++ - addr + 1;
        m_host = static_cast<char*>(malloc(size));
        memcpy(m_host, addr, size - 1);
        m_host[size - 1] = '\0';

        m_port = (uint16_t) strtol(port, nullptr, 10);
    }


    inline ~Addr()
    {
        free(m_host);
    }


    inline bool isValid() const     { return m_host && m_port > 0; }
    inline const char *host() const { return m_host; }
    inline uint16_t port() const    { return m_port; }

private:
    char *m_host;
    uint16_t m_port;
};

#endif /* __ADDR_H__ */
