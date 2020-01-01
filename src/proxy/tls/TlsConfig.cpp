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


#include "proxy/tls/TlsConfig.h"
#include "rapidjson/document.h"


/**
 * "cert"         load TLS certificate chain from file.
 * "cert_key"     load TLS private key from file.
 * "ciphers"      set list of available ciphers (TLSv1.2 and below).
 * "ciphersuites" set list of available TLSv1.3 ciphersuites.
 * "dhparam"      load DH parameters for DHE ciphers from file.
 */
xmrig::TlsConfig::TlsConfig(const rapidjson::Value &object)
{
    setProtocols(object["protocols"]);
    setCert(object["cert"].GetString());
    setKey(object["cert_key"].GetString());
    setCiphers(object["ciphers"].GetString());
    setCipherSuites(object["ciphersuites"].GetString());
    setDH(object["dhparam"].GetString());

    if (m_key.isNull()) {
        setKey(object["cert-key"].GetString());
    }
}


rapidjson::Value xmrig::TlsConfig::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;

    auto &allocator = doc.GetAllocator();
    Value obj(kObjectType);

    if (m_protocols > 0) {
        std::vector<String> protocols;

        if (m_protocols & TLSv1) {
            protocols.emplace_back("TLSv1");
        }

        if (m_protocols & TLSv1_1) {
            protocols.emplace_back("TLSv1.1");
        }

        if (m_protocols & TLSv1_2) {
            protocols.emplace_back("TLSv1.2");
        }

        if (m_protocols & TLSv1_3) {
            protocols.emplace_back("TLSv1.3");
        }

        obj.AddMember("protocols", String::join(protocols, ' ').toJSON(doc), allocator);
    }
    else {
        obj.AddMember("protocols", kNullType, allocator);
    }

    obj.AddMember("cert",         m_cert.toJSON(), allocator);
    obj.AddMember("cert_key",     m_key.toJSON(), allocator);
    obj.AddMember("ciphers",      m_ciphers.toJSON(), allocator);
    obj.AddMember("ciphersuites", m_cipherSuites.toJSON(), allocator);
    obj.AddMember("dhparam",      m_dhparam.toJSON(), allocator);

    return obj;
}


void xmrig::TlsConfig::setProtocols(const char *protocols)
{
    const std::vector<String> vec = String(protocols).split(' ');

    for (const String &value : vec) {
        if (value == "TLSv1") {
            m_protocols |= TLSv1;
        }
        else if (value == "TLSv1.1") {
            m_protocols |= TLSv1_1;
        }
        else if (value == "TLSv1.2") {
            m_protocols |= TLSv1_2;
        }
        else if (value == "TLSv1.3") {
            m_protocols |= TLSv1_3;
        }
    }
}


void xmrig::TlsConfig::setProtocols(const rapidjson::Value &protocols)
{
    m_protocols = 0;

    if (protocols.IsUint()) {
        return setProtocols(protocols.GetUint());
    }

    if (protocols.IsString()) {
        return setProtocols(protocols.GetString());
    }
}
