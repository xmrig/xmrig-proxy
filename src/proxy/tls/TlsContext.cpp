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
#include "proxy/tls/TlsConfig.h"
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


bool xmrig::TlsContext::load(const TlsConfig &config)
{
    if (m_ctx == nullptr) {
        LOG_ERR("Unable to create SSL context");

        return false;
    }

    if (!config.isValid()) {
        LOG_ERR("No valid TLS configuration provided");

        return false;
    }

    if (SSL_CTX_use_certificate_chain_file(m_ctx, config.cert()) <= 0) {
        LOG_ERR("SSL_CTX_use_certificate_chain_file(\"%s\") failed.", config.cert());

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(m_ctx, config.key(), SSL_FILETYPE_PEM) <= 0) {
        LOG_ERR("SSL_CTX_use_PrivateKey_file(\"%s\") failed.", config.key());

        return false;
    }

    SSL_CTX_set_options(m_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(m_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);

#   if OPENSSL_VERSION_NUMBER >= 0x1010100fL
    SSL_CTX_set_max_early_data(m_ctx, 0);
#   endif

    setProtocols(config.protocols());

    return setCiphers(config.ciphers()) && setCipherSuites(config.cipherSuites()) && setDH(config.dhparam());
}


bool xmrig::TlsContext::setCiphers(const char *ciphers)
{
    if (ciphers == nullptr || SSL_CTX_set_cipher_list(m_ctx, ciphers) == 1) {
        return true;
    }

    LOG_ERR("SSL_CTX_set_cipher_list(\"%s\") failed.", ciphers);

    return true;
}


bool xmrig::TlsContext::setCipherSuites(const char *ciphersuites)
{
    if (ciphersuites == nullptr) {
        return true;
    }

#   if OPENSSL_VERSION_NUMBER >= 0x1010100fL
    if (SSL_CTX_set_ciphersuites(m_ctx, ciphersuites) == 1) {
        return true;
    }
#   endif

    LOG_ERR("SSL_CTX_set_ciphersuites(\"%s\") failed.", ciphersuites);

    return false;
}


bool xmrig::TlsContext::setDH(const char *dhparam)
{
    if (dhparam == nullptr) {
        return true;
    }

    BIO *bio = BIO_new_file(dhparam, "r");
    if (bio == nullptr) {
        LOG_ERR("BIO_new_file(\"%s\") failed.", dhparam);

        return false;
    }

    DH *dh = PEM_read_bio_DHparams(bio, nullptr, nullptr, nullptr);
    if (dh == nullptr) {
        LOG_ERR("PEM_read_bio_DHparams(\"%s\") failed.", dhparam);

        BIO_free(bio);

        return false;
    }

    const int rc = SSL_CTX_set_tmp_dh(m_ctx, dh);

    DH_free(dh);
    BIO_free(bio);

    if (rc == 0) {
        LOG_ERR("SSL_CTX_set_tmp_dh(\"%s\") failed.", dhparam);

        return false;
    }

    return true;
}


void xmrig::TlsContext::setProtocols(uint32_t protocols)
{
    if (protocols == 0) {
        return;
    }

    if (!(protocols & TlsConfig::TLSv1)) {
        SSL_CTX_set_options(m_ctx, SSL_OP_NO_TLSv1);
    }

#   ifdef SSL_OP_NO_TLSv1_1
    SSL_CTX_clear_options(m_ctx, SSL_OP_NO_TLSv1_1);
    if (!(protocols & TlsConfig::TLSv1_1)) {
        SSL_CTX_set_options(m_ctx, SSL_OP_NO_TLSv1_1);
    }
#   endif

#   ifdef SSL_OP_NO_TLSv1_2
    SSL_CTX_clear_options(m_ctx, SSL_OP_NO_TLSv1_2);
    if (!(protocols & TlsConfig::TLSv1_2)) {
        SSL_CTX_set_options(m_ctx, SSL_OP_NO_TLSv1_2);
    }
#   endif

#   ifdef SSL_OP_NO_TLSv1_3
    SSL_CTX_clear_options(m_ctx, SSL_OP_NO_TLSv1_3);
    if (!(protocols & TlsConfig::TLSv1_3)) {
        SSL_CTX_set_options(m_ctx, SSL_OP_NO_TLSv1_3);
    }
#   endif
}
