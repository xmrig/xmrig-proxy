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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>


#include "log/Log.h"
#include "net/Job.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/Events.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/JobResult.h"
#include "proxy/LoginRequest.h"
#include "proxy/Miner.h"
#include "proxy/Uuid.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"


static int64_t nextId = 0;


Miner::Miner() :
    m_id(++nextId),
    m_loginId(0),
    m_recvBufPos(0),
    m_mapperId(-1),
    m_state(WaitLoginState),
    m_customDiff(0),
    m_diff(0),
    m_expire(uv_now(uv_default_loop()) + kLoginTimeout),
    m_rx(0),
    m_timestamp(uv_now(uv_default_loop())),
    m_tx(0),
    m_fixedByte(0)
{
    memset(m_ip, 0, sizeof(m_ip));
    Uuid::create(m_rpcId, sizeof(m_rpcId));

    m_socket.data = this;
    uv_tcp_init(uv_default_loop(), &m_socket);

    m_recvBuf.base = m_buf;
    m_recvBuf.len  = sizeof(m_buf);

    Counters::connections++;
}


Miner::~Miner()
{
    m_socket.data = nullptr;

    Counters::connections--;
}


bool Miner::accept(uv_stream_t *server)
{
    const int rt = uv_accept(server, reinterpret_cast<uv_stream_t*>(&m_socket));
    if (rt < 0) {
        LOG_ERR("[miner] accept error: \"%s\"", uv_strerror(rt));
        return false;
    }

    sockaddr_storage addr = { 0 };
    int size = sizeof(addr);

    uv_tcp_getpeername(&m_socket, reinterpret_cast<sockaddr*>(&addr), &size);
    uv_ip4_name(reinterpret_cast<sockaddr_in*>(&addr), m_ip, 16);

    uv_read_start(reinterpret_cast<uv_stream_t*>(&m_socket), Miner::onAllocBuffer, Miner::onRead);

    return true;
}


void Miner::replyWithError(int64_t id, const char *message)
{
    send(snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":{\"code\":-1,\"message\":\"%s\"}}\n", id, message));
}


void Miner::setJob(Job &job)
{
    snprintf(m_sendBuf, 4, "%02hhx", m_fixedByte);

    memcpy(job.rawBlob() + 84, m_sendBuf, 2);

    m_diff = job.diff();
    bool customDiff = false;

    char target[9];
    if (m_customDiff && m_customDiff < m_diff) {
        const uint64_t t = 0xFFFFFFFFFFFFFFFFULL / m_customDiff;
        Job::toHex(reinterpret_cast<const unsigned char *>(&t) + 4, 4, target);
        target[8] = '\0';
        customDiff = true;
    }

    int size = 0;
    if (m_state == WaitReadyState) {
        setState(ReadyState);
        size = snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"result\":{\"id\":\"%s\",\"job\":{\"blob\":\"%s\",\"job_id\":\"%s%02hhx0\",\"target\":\"%s\"},\"status\":\"OK\"}}\n",
                        m_loginId, m_rpcId, job.rawBlob(), job.id().data(), m_fixedByte, customDiff ? target : job.rawTarget());
    }
    else {
        size = snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"jsonrpc\":\"2.0\",\"method\":\"job\",\"params\":{\"blob\":\"%s\",\"job_id\":\"%s%02hhx0\",\"target\":\"%s\"}}\n",
                        job.rawBlob(), job.id().data(), m_fixedByte, customDiff ? target : job.rawTarget());
    }

    send(size);
}


void Miner::success(int64_t id, const char *status)
{
    send(snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"%s\"}}\n", id, status));
}


bool Miner::parseRequest(int64_t id, const char *method, const rapidjson::Value &params)
{
    if (!method || !params.IsObject()) {
        return false;
    }

    if (m_state == WaitLoginState) {
        if (strcmp(method, "login") == 0) {
            setState(WaitReadyState);
            m_loginId = id;

            LoginEvent::create(this, id, params["login"].GetString(), params["pass"].GetString(), params["agent"].GetString())->start();
            return true;
        }

        return false;
    }

    if (m_state == WaitReadyState) {
        return false;
    }

    if (strcmp(method, "submit") == 0) {
        heartbeat();

        const char *rpcId = params["id"].GetString();
        if (!rpcId || strncmp(m_rpcId, rpcId, sizeof(m_rpcId)) != 0) {
            replyWithError(id, Error::toString(Error::Unauthenticated));
            return true;
        }

        SubmitEvent *event = SubmitEvent::create(this, id, params["job_id"].GetString(), params["nonce"].GetString(), params["result"].GetString());

        if (!event->request.isValid() || event->request.actualDiff() < diff()) {
            event->reject(Error::LowDifficulty);
        }
        else if (!event->request.isCompatible(m_fixedByte)) {
            event->reject(Error::InvalidNonce);
        }

        if (event->error() == Error::NoError && m_customDiff && event->request.actualDiff() < m_diff) {
            success(id, "OK");
            return true;
        }

        if (!event->start()) {
            replyWithError(id, event->message());
        }

        return event->error() != Error::InvalidNonce;
    }

    if (strcmp(method, "keepalived") == 0) {
        heartbeat();
        success(id, "KEEPALIVED");
        return true;
    }

    replyWithError(id, Error::toString(Error::InvalidMethod));
    return true;
}


void Miner::heartbeat()
{
    m_expire = uv_now(uv_default_loop()) + kSocketTimeout;
}


void Miner::parse(char *line, size_t len)
{
    if (m_state == ClosingState) {
        return;
    }

    line[len - 1] = '\0';

    LOG_DEBUG("[%s] received (%d bytes): \"%s\"", m_ip, len, line);

    if (len < 32 || line[0] != '{') {
        return shutdown(true);
    }

    rapidjson::Document doc;
    if (doc.ParseInsitu(line).HasParseError()) {
        LOG_ERR("[%s] JSON decode failed: \"%s\"", m_ip, rapidjson::GetParseError_En(doc.GetParseError()));

        return shutdown(true);
    }

    if (!doc.IsObject()) {
        return shutdown(true);
    }

    const rapidjson::Value &id = doc["id"];
    if (id.IsInt64() && parseRequest(id.GetInt64(), doc["method"].GetString(), doc["params"])) {
        return;
    }

    shutdown(true);
}


void Miner::send(int size)
{
    LOG_DEBUG("[%s] send (%d bytes): \"%s\"", m_ip, size, m_sendBuf);

    if (size <= 0 || m_state != ReadyState || uv_is_writable(reinterpret_cast<uv_stream_t*>(&m_socket)) == 0) {
        return;
    }

    uv_buf_t buf = uv_buf_init(m_sendBuf, (unsigned int) size);
    const int rc = uv_try_write(reinterpret_cast<uv_stream_t*>(&m_socket), &buf, 1);

    if (rc < 0) {
        return shutdown(true);
    }
}


void Miner::setState(State state)
{
    if (m_state == state) {
        return;
    }

    if (state == ReadyState) {
        heartbeat();
        Counters::add();
    }

    if (state == ClosingState && m_state == ReadyState) {
        Counters::remove();
    }

    m_state = state;
}


void Miner::shutdown(bool had_error)
{
    if (m_state == ClosingState) {
        return;
    }

    setState(ClosingState);
    uv_read_stop(reinterpret_cast<uv_stream_t*>(&m_socket));

    uv_shutdown(new uv_shutdown_t, reinterpret_cast<uv_stream_t*>(&m_socket), [](uv_shutdown_t* req, int status) {

        if (uv_is_closing(reinterpret_cast<uv_handle_t*>(req->handle)) == 0) {
            uv_close(reinterpret_cast<uv_handle_t*>(req->handle), [](uv_handle_t *handle) {
                CloseEvent::start(getMiner(handle->data));
                delete static_cast<Miner*>(handle->data);
            });
        }

        delete req;
    });
}


void Miner::onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    auto miner = getMiner(handle->data);

    buf->base = &miner->m_recvBuf.base[miner->m_recvBufPos];
    buf->len  = miner->m_recvBuf.len - miner->m_recvBufPos;
}


void Miner::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    auto miner = getMiner(stream->data);
    if (nread < 0 || (size_t) nread > (sizeof(m_buf) - 8 - miner->m_recvBufPos)) {
        return miner->shutdown(nread != UV_EOF);;
    }

    miner->m_rx += nread;
    miner->m_recvBufPos += nread;

    char* end;
    char* start = buf->base;
    size_t remaining = miner->m_recvBufPos;

    while ((end = static_cast<char*>(memchr(start, '\n', remaining))) != nullptr) {
        end++;
        size_t len = end - start;
        miner->parse(start, len);

        remaining -= len;
        start = end;
    }

    if (remaining == 0) {
        miner->m_recvBufPos = 0;
        return;
    }

    if (start == buf->base) {
        return;
    }

    memcpy(buf->base, start, remaining);
    miner->m_recvBufPos = remaining;
}


void Miner::onTimeout(uv_timer_t *handle)
{
    auto miner = getMiner(handle->data);
    miner->m_recvBuf.base[sizeof(m_buf) - 1] = '\0';

    miner->shutdown(true);
}
