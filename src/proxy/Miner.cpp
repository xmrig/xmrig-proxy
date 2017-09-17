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
#include <string.h>


#include "Counters.h"
#include "log/Log.h"
#include "net/Job.h"
#include "proxy/Error.h"
#include "proxy/Events.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/JobResult.h"
#include "proxy/LoginRequest.h"
#include "proxy/Miner.h"
#include "proxy/Uuid.h"


static int64_t nextId = 0;


Miner::Miner() :
    m_id(++nextId),
    m_loginId(0),
    m_recvBufPos(0),
    m_mapperId(-1),
    m_state(WaitLoginState),
    m_realmId(0),
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

    m_recvBuf.base = new char[kRecvBufSize];
    m_recvBuf.len  = kRecvBufSize;

    memset(m_recvBuf.base, 0, kRecvBufSize);
}


Miner::~Miner()
{
    m_socket.data = nullptr;
    delete [] m_recvBuf.base;
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
    const size_t size = 64 + strlen(message);
    char *req = static_cast<char*>(malloc(size));

    snprintf(req, size, "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":{\"code\":-1,\"message\":\"%s\"}}\n", id, message);
    send(req);
}


void Miner::send(char *data)
{
    LOG_DEBUG("[%s] send (%d bytes): \"%s\"", m_ip, strlen(data), data);
    if (m_state != ReadyState) {
        return;
    }

    const size_t size = strlen(data);
    uv_buf_t buf = uv_buf_init(data, (unsigned int) size);

    uv_write_t *req = new uv_write_t;
    req->data = buf.base;

    m_tx += size;

    uv_write(req, reinterpret_cast<uv_stream_t*>(&m_socket), &buf, 1, [](uv_write_t *req, int status) {
        free(req->data);
        delete req;
    });
}


void Miner::setJob(Job &job)
{
    const size_t size = 384;
    char *req = static_cast<char*>(malloc(size));
    snprintf(req, 4, "%02hhx", m_fixedByte);

    memcpy(job.rawBlob() + 84, req, 2);

    if (m_state == WaitReadyState) {
        setState(ReadyState);
        snprintf(req, size, "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"result\":{\"id\":\"%s\",\"job\":{\"blob\":\"%s\",\"job_id\":\"%s%02hhx\",\"target\":\"%s\"},\"status\":\"OK\"}}\n",
                 m_loginId, m_rpcId, job.rawBlob(), job.id(), m_fixedByte, job.rawTarget());
    }
    else {
        snprintf(req, size, "{\"jsonrpc\":\"2.0\",\"method\":\"job\",\"params\":{\"blob\":\"%s\",\"job_id\":\"%s%02hhx\",\"target\":\"%s\"}}\n",
                 job.rawBlob(), job.id(), m_fixedByte, job.rawTarget());
    }

    send(req);
}


void Miner::success(int64_t id, const char *status)
{
    const size_t size = 96;
    char *req = static_cast<char*>(malloc(size));

    snprintf(req, size, "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"%s\"}}\n", id, status);
    send(req);
}


bool Miner::parseRequest(int64_t id, const char *method, const json_t *params)
{
    if (!method || !json_is_object(params)) {
        return false;
    }

    if (m_state == WaitLoginState) {
        if (strcmp(method, "login") == 0) {
            setState(WaitReadyState);
            LoginRequest request(id, json_string_value(json_object_get(params, "login")), json_string_value(json_object_get(params, "pass")), json_string_value(json_object_get(params, "agent")));
            m_loginId = id;

            LoginEvent::start(this, request);
            return true;
        }

        return false;
    }

    if (m_state == WaitReadyState) {
        return false;
    }

    if (strcmp(method, "submit") == 0) {
        heartbeat();

        const char *rpcId = json_string_value(json_object_get(params, "id"));
        if (!rpcId || strncmp(m_rpcId, rpcId, sizeof(m_rpcId)) != 0) {
            replyWithError(id, Error::toString(Error::Unauthenticated));
            return true;
        }

        JobResult request(id, json_string_value(json_object_get(params, "job_id")), json_string_value(json_object_get(params, "nonce")), json_string_value(json_object_get(params, "result")));
        SubmitEvent *event = SubmitEvent::create(this, request);

        if (!request.isValid()) {
            event->reject(Error::LowDifficulty);
        }
        else if (!request.isCompatible(m_fixedByte)) {
            event->reject(Error::InvalidNonce);
        }

        if (!event->start()) {
            replyWithError(id, event->message());
        }

        return true;
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

    if (strlen(line) < 32 || line[0] != '{') {
        return shutdown(true);
    }

    json_error_t err;
    json_t *val = json_loads(line, 0, &err);

    if (!val) {
        //LOG_ERR("[%s] JSON decode failed: \"%s\": \"%s\"", m_ip, err.text, line);
        return shutdown(true);
    }

    const json_t *id = json_object_get(val, "id");
    if (!json_is_integer(id) || !parseRequest(json_integer_value(id), json_string_value(json_object_get(val, "method")), json_object_get(val, "params"))) {
        shutdown(true);
    }

    json_decref(val);
}


void Miner::setState(State state)
{
    if (m_state == state) {
        return;
    }

    if (state == ReadyState) {
        heartbeat();
    }

    m_state = state;
}


void Miner::shutdown(bool had_error)
{
    if (m_state == ClosingState) {
        return;
    }

    setState(ClosingState);

    uv_shutdown_t* req = new uv_shutdown_t;
    uv_shutdown(req, reinterpret_cast<uv_stream_t*>(&m_socket), [](uv_shutdown_t* req, int status) {
        if (uv_is_closing(reinterpret_cast<uv_handle_t*>(req->handle)) == 0) {
            uv_close(reinterpret_cast<uv_handle_t*>(req->handle), [](uv_handle_t *handle) {
                delete static_cast<Miner*>(handle->data);
            });
        }

        delete req;
    });

    CloseEvent::start(this);
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
    if (nread < 0 || (size_t) nread > (kRecvBufSize - 8 - miner->m_recvBufPos)) {
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
    miner->m_recvBuf.base[kRecvBufSize - 1] = '\0';

    LOG_ERR("SHUTDOWN %s %s", miner->m_ip, miner->m_recvBuf.base);

    miner->shutdown(true);
}
