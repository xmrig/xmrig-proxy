/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
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


#include "proxy/tls/TlsConfig.h"
#include "rapidjson/document.h"


xmrig::TlsConfig::TlsConfig() :
    m_protocols(0)
{
}

xmrig::TlsConfig::TlsConfig(const rapidjson::Value &object) :
    m_protocols(0)
{
    setProtocols(object["protocols"]);
    setCert(object["cert"].GetString());
    setKey(object["cert_key"].GetString());
    setCiphers(object["ciphers"].GetString());
    setCipherSuites(object["ciphersuites"].GetString());
    setDH(object["dhparam"].GetString());
}


xmrig::TlsConfig::~TlsConfig()
{
}


rapidjson::Value xmrig::TlsConfig::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;

    auto &allocator = doc.GetAllocator();
    Value obj(kObjectType);

    if (m_protocols > 0) {
        Value protocols(kArrayType);

        if (m_protocols & TLSv1) {
            protocols.PushBack("TLSv1", allocator);
        }

        if (m_protocols & TLSv1_1) {
            protocols.PushBack("TLSv1.1", allocator);
        }

        if (m_protocols & TLSv1_2) {
            protocols.PushBack("TLSv1.2", allocator);
        }

        if (m_protocols & TLSv1_3) {
            protocols.PushBack("TLSv1.3", allocator);
        }

        obj.AddMember("protocols", protocols, allocator);
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


void xmrig::TlsConfig::setProtocols(const rapidjson::Value &protocols)
{
    m_protocols = 0;

    if (protocols.IsUint()) {
        return setProtocols(protocols.GetUint());
    }

    if (!protocols.IsArray()) {
        return;
    }

    for (const rapidjson::Value &value : protocols.GetArray()) {
        const char *protocol = value.GetString();
        if (protocol == nullptr) {
            continue;
        }

        if (strcmp(protocol, "TLSv1") == 0) {
            m_protocols |= TLSv1;
        }
        else if (strcmp(protocol, "TLSv1.1") == 0) {
            m_protocols |= TLSv1_1;
        }
        else if (strcmp(protocol, "TLSv1.2") == 0) {
            m_protocols |= TLSv1_2;
        }
        else if (strcmp(protocol, "TLSv1.3") == 0) {
            m_protocols |= TLSv1_3;
        }
    }
}
