/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright 2018      SChernykh   <https://github.com/SChernykh>
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


#include <assert.h>


#include "proxy/tls/Tls.h"


Miner::Tls::Tls(SSL_CTX *ctx, Miner *miner) :
    m_ready(false),
    m_buf(),
    m_fingerprint(),
    m_miner(miner),
    m_ssl(nullptr),
    m_ctx(ctx)
{
    m_writeBio = BIO_new(BIO_s_mem());
    m_readBio  = BIO_new(BIO_s_mem());
}


Miner::Tls::~Tls()
{
    if (m_ssl) {
        SSL_free(m_ssl);
    }
}


bool Miner::Tls::accept()
{
    m_ssl = SSL_new(m_ctx);
    assert(m_ssl != nullptr);

    if (!m_ssl) {
        return false;
    }

    SSL_set_accept_state(m_ssl);
    SSL_set_bio(m_ssl, m_readBio, m_writeBio);

    return send();
}


bool Miner::Tls::send(const char *data, size_t size)
{
    SSL_write(m_ssl, data, size);

    return send();
}


const char *Miner::Tls::fingerprint() const
{
    return m_ready ? m_fingerprint : nullptr;
}


const char *Miner::Tls::version() const
{
    return m_ready ? SSL_get_version(m_ssl) : nullptr;
}


void Miner::Tls::read(const char *data, size_t size)
{
    BIO_write(m_readBio, data, size);

    if (!SSL_is_init_finished(m_ssl)) {
        const int rc = SSL_do_handshake(m_ssl);

        if (rc < 0 && SSL_get_error(m_ssl, rc) == SSL_ERROR_WANT_READ) {
            send();
        } else if (rc == 1) {
            m_ready = true;
            send();
            read();
        }
        else {
            m_miner->close();
        }

      return;
    }

    read();
}


bool Miner::Tls::send()
{
    return m_miner->send(m_writeBio);
}


void  Miner::Tls::read()
{
    int bytes_read = 0;
    while ((bytes_read = SSL_read(m_ssl, m_buf, sizeof(m_buf))) > 0) {
        m_miner->parse(m_buf, bytes_read);
    }
}
