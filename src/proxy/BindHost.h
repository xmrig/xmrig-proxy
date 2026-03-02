/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_BINDHOST_H
#define XMRIG_BINDHOST_H


#include <stdint.h>
#include <vector>


#include "3rdparty/rapidjson/fwd.h"
#include "base/tools/String.h"


namespace xmrig {


class BindHost
{
public:
    constexpr static uint16_t kDefaultPort = 3333;


    inline BindHost() :
        m_tls(false),
        m_version(0),
        m_port(0)
    {}


    BindHost(const char *addr);
    BindHost(const char *host, uint16_t port, int version);
    BindHost(const rapidjson::Value &object);

    rapidjson::Value toJSON(rapidjson::Document &doc) const;

    inline bool isIPv6() const      { return m_version == 6; }
    inline bool isTLS() const       { return m_tls; }
    inline bool isValid() const     { return m_version && !m_host.isNull() && m_port > 0; }
    inline const char *host() const { return m_host.data(); }
    inline uint16_t port() const    { return m_port; }
    inline void setTLS(bool enable) { m_tls = enable; }

private:
    bool parseHost(const char *host);
    void parseIPv4(const char *addr);
    void parseIPv6(const char *addr);

    bool m_tls;
    int m_version;
    uint16_t m_port;
    String m_host;
};


typedef std::vector<BindHost> BindHosts;


} /* namespace xmrig */

#endif /* XMRIG_BINDHOST_H */
