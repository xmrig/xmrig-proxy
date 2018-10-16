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


#include <openssl/ssl.h>
#include <openssl/err.h>


#include "common/log/Log.h"
#include "proxy/tls/TlsContext.h"


xmrig::TlsContext::TlsContext() :
    m_ctx(nullptr)
{
    m_ctx = SSL_CTX_new(SSLv23_server_method());
}


xmrig::TlsContext::~TlsContext()
{
    SSL_CTX_free(m_ctx);
}


bool xmrig::TlsContext::load(const char *cert, const char *key)
{
    if (m_ctx == nullptr) {
        LOG_ERR("Unable to create SSL context");

        return false;
    }

    if (SSL_CTX_use_certificate_chain_file(m_ctx, cert) <= 0) {
        LOG_ERR("Failed to load certificate chain file \"%s\"", cert);

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(m_ctx, key, SSL_FILETYPE_PEM) <= 0) {
        LOG_ERR("Failed to load private key file \"%s\"", key);

        return false;
    }

    SSL_CTX_set_options(m_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    return true;
}
