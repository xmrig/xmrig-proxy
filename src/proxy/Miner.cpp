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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>


#include "common/log/Log.h"
#include "common/net/Job.h"
#include "common/utils/timestamp.h"
#include "net/JobResult.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/Events.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/Uuid.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


static int64_t nextId = 0;
xmrig::Storage<Miner> Miner::m_storage;


Miner::Miner(bool nicehash, bool ipv6) :
    m_ipv6(ipv6),
    m_nicehash(nicehash),
    m_ip(),
    m_id(++nextId),
    m_loginId(0),
    m_recvBufPos(0),
    m_mapperId(-1),
    m_state(WaitLoginState),
    m_customDiff(0),
    m_diff(0),
    m_expire(uv_now(uv_default_loop()) + kLoginTimeout),
    m_rx(0),
    m_timestamp(xmrig::currentMSecsSinceEpoch()),
    m_tx(0),
    m_fixedByte(0)
{
    m_key = m_storage.add(this);

    Uuid::create(m_rpcId, sizeof(m_rpcId));

    m_socket.data = m_storage.ptr(m_key);
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

    if (m_ipv6) {
        uv_ip6_name(reinterpret_cast<sockaddr_in6*>(&addr), m_ip, 45);
    } else {
        uv_ip4_name(reinterpret_cast<sockaddr_in*>(&addr), m_ip, 16);
    }

    uv_read_start(reinterpret_cast<uv_stream_t*>(&m_socket), Miner::onAllocBuffer, Miner::onRead);

    return true;
}


void Miner::replyWithError(int64_t id, const char *message)
{
    send(snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":{\"code\":-1,\"message\":\"%s\"}}\n", id, message));
}


void Miner::setJob(Job &job)
{
    using namespace rapidjson;

    if (m_nicehash) {
        snprintf(m_sendBuf, 4, "%02hhx", m_fixedByte);
        memcpy(job.rawBlob() + 84, m_sendBuf, 2);
    }

    m_diff = job.diff();
    bool customDiff = false;

    if (m_customDiff && m_customDiff < m_diff) {
        const uint64_t t = 0xFFFFFFFFFFFFFFFFULL / m_customDiff;
        Job::toHex(reinterpret_cast<const unsigned char *>(&t) + 4, 4, m_sendBuf);
        m_sendBuf[8] = '\0';
        customDiff = true;
    }

    sprintf(m_sendBuf + 16, "%s%02hhx0", job.id().data(), m_fixedByte);

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kObjectType);
    params.AddMember("blob",   StringRef(job.rawBlob()), allocator);
    params.AddMember("job_id", StringRef(m_sendBuf + 16), allocator);
    params.AddMember("target", StringRef(customDiff ? m_sendBuf : job.rawTarget()), allocator);
    params.AddMember("algo",   StringRef(job.algorithm().shortName()), allocator);

    if (job.algorithm().variant() == xmrig::VARIANT_0 || job.algorithm().variant() == xmrig::VARIANT_1) {
        params.AddMember("variant", job.algorithm().variant(), allocator);
    }

    doc.AddMember("jsonrpc", "2.0", allocator);

    if (m_state == WaitReadyState) {
        setState(ReadyState);

        doc.AddMember("id",    m_loginId, allocator);
        doc.AddMember("error", kNullType, allocator);

        Value result(kObjectType);
        result.AddMember("id",  StringRef(m_rpcId), allocator);
        result.AddMember("job", params, allocator);

        Value extensions(kArrayType);
        extensions.PushBack("algo", allocator);

        if (m_nicehash) {
            extensions.PushBack("nicehash", allocator);
        }

        result.AddMember("extensions", extensions, allocator);
        result.AddMember("status", "OK", allocator);

        doc.AddMember("result", result, allocator);
    }
    else {
        doc.AddMember("method", "job", allocator);
        doc.AddMember("params", params, allocator);
    }

    StringBuffer buffer(0, 512);
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    const size_t size = buffer.GetSize();
    if (size > (sizeof(m_sendBuf) - 2)) {
        return;
    }

    memcpy(m_sendBuf, buffer.GetString(), size);
    m_sendBuf[size]     = '\n';
    m_sendBuf[size + 1] = '\0';

    send(size + 1);
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

            xmrig::Algorithms algorithms;
            if (params.HasMember("algo")) {
                const rapidjson::Value &value = params["algo"];

                if (value.IsArray()) {
                    for (const rapidjson::Value &algo : value.GetArray()) {
                        algorithms.push_back(algo.GetString());
                    }
                }
            }

            m_user     = params["login"].GetString();
            m_password = params["pass"].GetString();
            m_agent    = params["agent"].GetString();
            m_rigId    = params["rigid"].GetString();

            LoginEvent::create(this, id, m_user.data(), m_password.data(), m_agent.data(), m_rigId.data(), algorithms)->start();
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

        xmrig::Algorithm algorithm;
        if (params.HasMember("algo")) {
            const char *algo = params["algo"].GetString();

            algorithm.parseAlgorithm(algo);
            if (!algorithm.isValid()) {
                algorithm.parseXmrStakAlgorithm(algo);
            }
        }

        SubmitEvent *event = SubmitEvent::create(this, id, params["job_id"].GetString(), params["nonce"].GetString(), params["result"].GetString(), algorithm);

        if (!event->request.isValid() || event->request.actualDiff() < diff()) {
            event->reject(Error::LowDifficulty);
        }
        else if (m_nicehash && !event->request.isCompatible(m_fixedByte)) {
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

    if (size <= 0 || (m_state != ReadyState && m_state != WaitReadyState) || uv_is_writable(reinterpret_cast<uv_stream_t*>(&m_socket)) == 0) {
        return;
    }

    uv_buf_t buf = uv_buf_init(m_sendBuf, (unsigned int) size);
    const int rc = uv_try_write(reinterpret_cast<uv_stream_t*>(&m_socket), &buf, 1);

    if (rc < 0) {
        return shutdown(true);
    }

    m_tx += size;
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
                Miner *miner = getMiner(handle->data);
                if (!miner) {
                    return;
                }

                CloseEvent::start(miner);
                m_storage.remove(handle->data);
            });
        }

        delete req;
    });
}


void Miner::onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    auto miner = getMiner(handle->data);
    if (!miner) {
        return;
    }

    buf->base = &miner->m_recvBuf.base[miner->m_recvBufPos];
    buf->len  = miner->m_recvBuf.len - miner->m_recvBufPos;
}


void Miner::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    auto miner = getMiner(stream->data);
    if (!miner) {
        return;
    }

    if (nread < 0 || (size_t) nread > (sizeof(m_buf) - 8 - miner->m_recvBufPos)) {
        return miner->shutdown(nread != UV_EOF);;
    }

    miner->m_rx += nread;
    miner->m_recvBufPos += nread;

    char* end;
    char* start = miner->m_recvBuf.base;
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

    if (start == miner->m_recvBuf.base) {
        return;
    }

    memcpy(miner->m_recvBuf.base, start, remaining);
    miner->m_recvBufPos = remaining;
}


void Miner::onTimeout(uv_timer_t *handle)
{
    auto miner = getMiner(handle->data);
    if (!miner) {
        return;
    }

    miner->m_recvBuf.base[sizeof(m_buf) - 1] = '\0';

    miner->shutdown(true);
}
