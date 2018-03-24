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
        m_addr(nullptr),
        m_ip(nullptr),
        m_version(0),
        m_port(0)
    {}


    inline Addr(const char *addr) :
        m_addr(strdup(addr)),
        m_ip(nullptr),
        m_version(0),
        m_port(0)
    {
        if (!addr || strlen(addr) < 5) {
            return;
        }

        if (addr[0] == '[') {
            parseIPv6(addr);
            return;
        }

        parseIPv4(addr);
    }


    inline ~Addr()
    {
        delete [] m_addr;
        delete [] m_ip;
    }


    inline bool isIPv6() const      { return m_version == 6; }
    inline bool isValid() const     { return m_version && m_ip && m_port > 0; }
    inline const char *addr() const { return m_addr; }
    inline const char *ip() const   { return m_ip; }
    inline uint16_t port() const    { return m_port; }

private:
    void parseIPv4(const char *addr)
    {
        const char *port = strchr(addr, ':');
        if (!port) {
            return;
        }

        m_version = 4;
        const size_t size = port++ - addr + 1;
        m_ip = new char[size]();
        memcpy(m_ip, addr, size - 1);

        m_port = (uint16_t) strtol(port, nullptr, 10);
    }


    void parseIPv6(const char *addr)
    {
        const char *end = strchr(addr, ']');
        if (!end) {
            return;
        }

        const char *port = strchr(end, ':');
        if (!port) {
            return;
        }

        m_version = 6;
        const size_t size = end - addr;
        m_ip = new char[size]();
        memcpy(m_ip, addr + 1, size - 1);

        m_port = (uint16_t) strtol(port + 1, nullptr, 10);
    }


    char *m_addr;
    char *m_ip;
    int m_version;
    uint16_t m_port;
};

#endif /* __ADDR_H__ */
