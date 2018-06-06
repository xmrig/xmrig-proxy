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

#ifndef __POOL_H__
#define __POOL_H__


#include <vector>


#include "common/crypto/Algorithm.h"
#include "common/utils/c_str.h"
#include "rapidjson/fwd.h"


class Pool
{
public:
    constexpr static const char *kDefaultPassword = "x";
    constexpr static const char *kDefaultUser     = "x";
    constexpr static uint16_t kDefaultPort        = 3333;
    constexpr static int kKeepAliveTimeout        = 60;

    Pool();
    Pool(const char *url);
    Pool(const char *host,
        uint16_t port,
        const char *user       = nullptr,
        const char *password   = nullptr,
        int keepAlive          = 0,
        bool nicehash          = false
       );

    inline bool isNicehash() const                     { return m_nicehash; }
    inline bool isValid() const                        { return !m_host.isNull() && m_port > 0; }
    inline const char *host() const                    { return m_host.data(); }
    inline const char *password() const                { return !m_password.isNull() ? m_password.data() : kDefaultPassword; }
    inline const char *rigId() const                   { return m_rigId.data(); }
    inline const char *url() const                     { return m_url.data(); }
    inline const char *user() const                    { return !m_user.isNull() ? m_user.data() : kDefaultUser; }
    inline const xmrig::Algorithm &algorithm() const   { return m_algorithm; }
    inline const xmrig::Algorithms &algorithms() const { return m_algorithms; }
    inline int keepAlive() const                       { return m_keepAlive; }
    inline uint16_t port() const                       { return m_port; }
    inline void setKeepAlive(int keepAlive)            { m_keepAlive = keepAlive >= 0 ? keepAlive : 0; }
    inline void setNicehash(bool nicehash)             { m_nicehash = nicehash; }
    inline void setPassword(const char *password)      { m_password = password; }
    inline void setRigId(const char *rigId)            { m_rigId = rigId; }
    inline void setUser(const char *user)              { m_user = user; }
    inline xmrig::Algorithm &algorithm()               { return m_algorithm; }

    inline bool operator!=(const Pool &other) const  { return !isEqual(other); }
    inline bool operator==(const Pool &other) const  { return isEqual(other); }

    bool isCompatible(const xmrig::Algorithm &algorithm) const;
    bool isEqual(const Pool &other) const;
    bool parse(const char *url);
    bool setUserpass(const char *userpass);
    rapidjson::Value toJSON(rapidjson::Document &doc) const;
    void adjust(xmrig::Algo algorithm);
    void setAlgo(const xmrig::Algorithm &algorithm);

#   ifdef APP_DEBUG
    void print() const;
#   endif

private:
    bool parseIPv6(const char *addr);
    void addVariant(xmrig::Variant variant);
    void rebuild();

    bool m_nicehash;
    int m_keepAlive;
    uint16_t m_port;
    xmrig::Algorithm m_algorithm;
    xmrig::Algorithms m_algorithms;
    xmrig::c_str m_host;
    xmrig::c_str m_password;
    xmrig::c_str m_rigId;
    xmrig::c_str m_url;
    xmrig::c_str m_user;
};


typedef std::vector<Pool> Pools;

#endif /* __POOL_H__ */
