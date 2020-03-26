/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
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

#ifndef XMRIG_TLSCONFIG_H
#define XMRIG_TLSCONFIG_H


#include "base/tools/String.h"
#include "rapidjson/fwd.h"


namespace xmrig {


class TlsConfig
{
public:
    enum Versions {
        TLSv1   = 1,
        TLSv1_1 = 2,
        TLSv1_2 = 4,
        TLSv1_3 = 8
    };

    TlsConfig() = default;
    TlsConfig(const rapidjson::Value &object);

    inline bool isValid() const                      { return !m_cert.isEmpty() && !m_key.isEmpty(); }
    inline const char *cert() const                  { return m_cert.data(); }
    inline const char *ciphers() const               { return m_ciphers.isEmpty() ? nullptr : m_ciphers.data(); }
    inline const char *cipherSuites() const          { return m_cipherSuites.isEmpty() ? nullptr : m_cipherSuites.data(); }
    inline const char *dhparam() const               { return m_dhparam.isEmpty() ? nullptr : m_dhparam.data(); }
    inline const char *key() const                   { return m_key.data(); }
    inline uint32_t protocols() const                { return m_protocols; }
    inline void setCert(const char *cert)            { m_cert = cert; }
    inline void setCiphers(const char *ciphers)      { m_ciphers = ciphers; }
    inline void setCipherSuites(const char *ciphers) { m_cipherSuites = ciphers; }
    inline void setDH(const char *dhparam)           { m_dhparam = dhparam; }
    inline void setKey(const char *key)              { m_key = key; }
    inline void setProtocols(uint32_t protocols)     { m_protocols = protocols; }

    rapidjson::Value toJSON(rapidjson::Document &doc) const;
    void setProtocols(const char *protocols);
    void setProtocols(const rapidjson::Value &protocols);

private:
    uint32_t m_protocols = 0;
    String m_cert;
    String m_ciphers;
    String m_cipherSuites;
    String m_dhparam;
    String m_key;
};


} /* namespace xmrig */

#endif /* XMRIG_TLSCONFIG_H */
