/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2025 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2025 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "proxy/Miner.h"
#include "3rdparty/rapidjson/document.h"
#include "3rdparty/rapidjson/error/en.h"
#include "3rdparty/rapidjson/stringbuffer.h"
#include "3rdparty/rapidjson/writer.h"
#include "base/io/json/Json.h"
#include "base/io/log/Log.h"
#include "base/net/stratum/Job.h"
#include "base/net/tools/NetBuffer.h"
#include "base/tools/Cvt.h"
#include "base/tools/Chrono.h"
#include "net/JobResult.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"


#ifdef XMRIG_FEATURE_TLS
#   include "base/net/tls/TlsContext.h"
#   include "proxy/tls/MinerTls.h"
#   include <openssl/bio.h>
#endif


#include <cinttypes>
#include <cstdio>
#include <cstring>


namespace xmrig {
    static int64_t nextId = 0;
    char Miner::m_sendBuf[16384] = { 0 };
    Storage<Miner> Miner::m_storage;
}


xmrig::Miner::Miner(const TlsContext *ctx, uint16_t port, bool strictTls) :
    m_strictTls(strictTls),
    m_rpcId(Cvt::toHex(Cvt::randomBytes(8))),
    m_tlsCtx(ctx),
    m_id(++nextId),
    m_localPort(port),
    m_expire(Chrono::currentMSecsSinceEpoch() + kLoginTimeout),
    m_timestamp(Chrono::currentMSecsSinceEpoch())
{
    m_reader.setListener(this);
    m_key = m_storage.add(this);

    m_socket = new uv_tcp_t;
    m_socket->data = m_storage.ptr(m_key);
    uv_tcp_init(uv_default_loop(), m_socket);

    Counters::connections++;
}


xmrig::Miner::~Miner()
{
    if (uv_is_closing(reinterpret_cast<uv_handle_t *>(m_socket))) {
        delete m_socket;
    }
    else {
        uv_close(reinterpret_cast<uv_handle_t *>(m_socket), [](uv_handle_t *handle) { delete reinterpret_cast<uv_tcp_t *>(handle); });
    }

#   ifdef XMRIG_FEATURE_TLS
    delete m_tls;
#   endif

    Counters::connections--;
}


bool xmrig::Miner::accept(uv_stream_t *server)
{
    const int rt = uv_accept(server, reinterpret_cast<uv_stream_t*>(m_socket));
    if (rt < 0) {
        LOG_ERR("[miner] accept error: \"%s\"", uv_strerror(rt));
        return false;
    }

    sockaddr_storage addr = {};
    int size = sizeof(addr);

    uv_tcp_getpeername(m_socket, reinterpret_cast<sockaddr*>(&addr), &size);

    if (reinterpret_cast<sockaddr_in *>(&addr)->sin_family == AF_INET6) {
        uv_ip6_name(reinterpret_cast<sockaddr_in6*>(&addr), m_ip, 45);
    } else {
        uv_ip4_name(reinterpret_cast<sockaddr_in*>(&addr), m_ip, 16);
    }

    uv_read_start(reinterpret_cast<uv_stream_t*>(m_socket), NetBuffer::onAlloc, Miner::onRead);

    return true;
}


void xmrig::Miner::forwardJob(const Job &job, const char *algo)
{
    m_diff = job.diff();
    setFixedByte(job.fixedByte());

    sendJob(job.rawBlob(), job.id().data(), job.rawTarget(), algo ? algo : job.algorithm().name(), job.height(), job.rawSeedHash(), job.rawSigKey());
}


void xmrig::Miner::replyWithError(int64_t id, const char *message)
{
    send(snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":{\"code\":-1,\"message\":\"%s\"}}\n", id, message));
}


void xmrig::Miner::setJob(Job &job, int64_t extra_nonce)
{
    using namespace rapidjson;

    if (hasExtension(EXT_NICEHASH)) {
        snprintf(m_sendBuf, 4, "%02hhx", m_fixedByte);
        memcpy(job.rawBlob() + (job.nonceOffset() + 3) * 2, m_sendBuf, 2);
    }

    m_diff = job.diff();
    bool customDiff = false;

    if (m_customDiff && m_customDiff < m_diff) {
        const uint64_t t = 0xFFFFFFFFFFFFFFFFULL / m_customDiff;
        Cvt::toHex(m_sendBuf, 9, reinterpret_cast<const uint8_t *>(&t) + 4, 4);
        customDiff = true;
    }

    const char* blob = job.rawBlob();
    String tmp_blob;

    if (job.hasMinerSignature()) {
        job.generateSignatureData(m_signatureData, m_viewTag);
    }
    else if (!job.rawSigKey().isNull()) {
        m_signatureData = job.rawSigKey();
    }

    if (job.hasViewTag()) {
        job.setViewTagInMinerTx(m_viewTag);
    }

    if (extra_nonce >= 0) {
        m_extraNonce = extra_nonce;
        job.setExtraNonceInMinerTx(static_cast<uint32_t>(m_extraNonce));
    }

    if (job.hasMinerSignature() || (extra_nonce >= 0)) {
        job.generateHashingBlob(tmp_blob);
        blob = tmp_blob;
    }

    sendJob(blob, job.id().data(), customDiff ? m_sendBuf : job.rawTarget(), job.algorithm().name(), job.height(), job.rawSeedHash(), m_signatureData);
}


void xmrig::Miner::success(int64_t id, const char *status)
{
    send(snprintf(m_sendBuf, sizeof(m_sendBuf), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"%s\"}}\n", id, status));
}


bool xmrig::Miner::isWritable() const
{
    return m_state != ClosingState && uv_is_writable(reinterpret_cast<const uv_stream_t*>(m_socket)) == 1;
}


bool xmrig::Miner::parseRequest(int64_t id, const char *method, const rapidjson::Value &params)
{
    if (!method || !params.IsObject()) {
        return false;
    }

    if (m_state == WaitLoginState) {
        if (strcmp(method, "login") == 0) {
            setState(WaitReadyState);
            m_loginId = id;

            Algorithms algorithms;
            if (params.HasMember("algo")) {
                const rapidjson::Value &value = params["algo"];

                if (value.IsArray()) {
                    algorithms.reserve(value.Size());

                    for (const auto &i : value.GetArray()) {
                        Algorithm algo(i.GetString());
                        if (!algo.isValid()) {
                            continue;
                        }

                        algorithms.emplace_back(algo);
                    }
                }
            }

            m_user     = Json::getString(params, "login");
            m_password = Json::getString(params, "pass");
            m_agent    = Json::getString(params, "agent");
            m_rigId    = Json::getString(params, "rigid");

            LoginEvent::create(this, id, algorithms, params)->start();
            return true;
        }

        return false;
    }

    if (m_state == WaitReadyState) {
        return false;
    }

    if (strcmp(method, "submit") == 0) {
        heartbeat();

        const char *rpcId = Json::getString(params, "id");
        if (!rpcId || m_rpcId != rpcId) {
            replyWithError(id, Error::toString(Error::Unauthenticated));
            return true;
        }

        Algorithm algorithm(Json::getString(params, "algo"));

        SubmitEvent *event = SubmitEvent::create(this, id, Json::getString(params, "job_id"), Json::getString(params, "nonce"), Json::getString(params, "result"), algorithm, Json::getString(params, "sig"), m_signatureData, m_viewTag, m_extraNonce);

        if (!event->request.isValid() || event->request.actualDiff() < diff()) {
            event->setError(Error::LowDifficulty);
        }
        else if (hasExtension(EXT_NICEHASH) && !event->request.isCompatible(m_fixedByte)) {
            event->setError(Error::InvalidNonce);
        }

        if (event->error() == Error::NoError && m_customDiff && event->request.actualDiff() < m_diff) {
            success(id, "OK");

            SubmitResult result = SubmitResult(1, m_customDiff, event->request.actualDiff(), event->request.id, 0);
            AcceptEvent::start(m_mapperId, this, result, false, true);

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


bool xmrig::Miner::send(BIO *bio)
{
#   ifdef XMRIG_FEATURE_TLS
    uv_buf_t buf;
    buf.len = BIO_get_mem_data(bio, &buf.base);

    if (buf.len == 0) {
        return true;
    }

    LOG_DEBUG("[%s] TLS send     (%d bytes)", m_ip, static_cast<int>(buf.len));

    if (!isWritable()) {
        return false;
    }

    const int rc = uv_try_write(reinterpret_cast<uv_stream_t*>(m_socket), &buf, 1);
    (void) BIO_reset(bio);

    if (rc < 0) {
        shutdown(true);

        return false;
    }

    m_tx += buf.len;

    return true;
#   else
    return false;
#   endif
}


void xmrig::Miner::heartbeat()
{
    m_expire = Chrono::currentMSecsSinceEpoch() + kSocketTimeout;
}


void xmrig::Miner::parse(char *line, size_t len)
{
    if (m_state == ClosingState) {
        return;
    }

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


void xmrig::Miner::read(ssize_t nread, const uv_buf_t *buf)
{
    const auto size = static_cast<size_t>(nread);

    if (nread < 0) {
        return shutdown(nread != UV_EOF);
    }

    if (size && m_rx == 0) {
        startTLS(buf->base);
    }

    m_rx += size;

#   ifdef XMRIG_FEATURE_TLS
    if (isTLS()) {
        LOG_DEBUG("[%s] TLS received (%d bytes)", m_ip, nread);

        m_tls->read(buf->base, size);
    }
    else
    {
        m_reader.parse(buf->base, size);
    }
#   else
    m_reader.parse(buf->base, size);
#   endif
}


void xmrig::Miner::send(const rapidjson::Document &doc)
{
    using namespace rapidjson;

    StringBuffer buffer(nullptr, 512);
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    const size_t size = buffer.GetSize();
    if (size > (sizeof(m_sendBuf) - 2)) {
        LOG_ERR("[%s] send failed: \"send buffer overflow: %zu > %zu\"", m_ip, size, (sizeof(m_sendBuf) - 2));
        shutdown(true);

        return;
    }

    memcpy(m_sendBuf, buffer.GetString(), size);
    m_sendBuf[size]     = '\n';
    m_sendBuf[size + 1] = '\0';

    return send(size + 1);
}


void xmrig::Miner::send(int size)
{
    LOG_DEBUG("[%s] send (%d bytes): \"%s\"", m_ip, size, m_sendBuf);

    if (size <= 0 || !isWritable()) {
        return;
    }

    int rc = -1;
#   ifdef XMRIG_FEATURE_TLS
    if (isTLS()) {
        rc = m_tls->send(m_sendBuf, size) ? 0 : -1;
    }
    else
#   endif
    {
        uv_buf_t buf = uv_buf_init(m_sendBuf, (unsigned int) size);
        rc = uv_try_write(reinterpret_cast<uv_stream_t*>(m_socket), &buf, 1);
    }

    if (rc < 0) {
        return shutdown(true);
    }

    m_tx += size;
}


void xmrig::Miner::sendJob(const char *blob, const char *jobId, const char *target, const char *algo, uint64_t height, const String &seedHash, const String &signatureKey)
{
    using namespace rapidjson;

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kObjectType);
    params.AddMember("blob",   StringRef(blob), allocator);
    params.AddMember("job_id", StringRef(jobId), allocator);
    params.AddMember("target", StringRef(target), allocator);
    params.AddMember("algo",   StringRef(algo), allocator);

    if (height) {
        params.AddMember("height", height, allocator);
    }

    if (!seedHash.isNull()) {
        params.AddMember("seed_hash", seedHash.toJSON(), allocator);
    }

    if (!signatureKey.isNull()) {
        // Skip tx_pubkey (first 32 bytes) because client doesn't need it for signing
        const char *key = signatureKey.size() == 192 ? (signatureKey.data() + 64) : signatureKey.data();
        params.AddMember("sig_key", Value(key, allocator), allocator);
    }

    doc.AddMember("jsonrpc", "2.0", allocator);

    if (m_state == WaitReadyState) {
        setState(ReadyState);

        doc.AddMember("id",    m_loginId, allocator);
        doc.AddMember("error", kNullType, allocator);

        Value result(kObjectType);
        result.AddMember("id",  m_rpcId.toJSON(), allocator);
        result.AddMember("job", params, allocator);

        Value extensions(kArrayType);

        if (hasExtension(EXT_ALGO)) {
            extensions.PushBack("algo", allocator);
        }

        if (hasExtension(EXT_NICEHASH)) {
            extensions.PushBack("nicehash", allocator);
        }

        if (hasExtension(EXT_CONNECT)) {
            extensions.PushBack("connect", allocator);

#           ifdef XMRIG_FEATURE_TLS
            extensions.PushBack("tls", allocator);
#           endif
        }

        extensions.PushBack("keepalive", allocator);

        result.AddMember("extensions", extensions, allocator);
        result.AddMember("status", "OK", allocator);

        doc.AddMember("result", result, allocator);
    }
    else {
        doc.AddMember("method", "job", allocator);
        doc.AddMember("params", params, allocator);
    }

    send(doc);
}


void xmrig::Miner::setState(State state)
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


void xmrig::Miner::shutdown(bool had_error)
{
    if (m_state == ClosingState) {
        return;
    }

    setState(ClosingState);
    uv_read_stop(reinterpret_cast<uv_stream_t*>(m_socket));

    // uv_shutdown gets stuck when the connection was not terminated gracefully
    if (had_error) {
        if (uv_is_closing(reinterpret_cast<uv_handle_t*>(m_socket)) == 0) {
            uv_close(reinterpret_cast<uv_handle_t*>(m_socket), [](uv_handle_t* handle) {
                Miner* miner = getMiner(handle->data);
                if (!miner) {
                    return;
                }

                CloseEvent::start(miner);
                m_storage.remove(handle->data);
            });
        }
        return;
    }

    uv_shutdown(new uv_shutdown_t, reinterpret_cast<uv_stream_t*>(m_socket), [](uv_shutdown_t* req, int) {

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


void xmrig::Miner::startTLS(const char *data)
{
#   ifdef XMRIG_FEATURE_TLS
    if (m_tlsCtx && (m_strictTls || *data != '{')) {
        m_tls = new Tls(m_tlsCtx->ctx(), this);
    }
#   endif
}


void xmrig::Miner::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    auto miner = getMiner(stream->data);
    if (miner) {
        miner->read(nread, buf);
    }

    NetBuffer::release(buf);
}


void xmrig::Miner::onTimeout(uv_timer_t *handle)
{
    auto miner = getMiner(handle->data);
    if (!miner) {
        return;
    }

    miner->shutdown(true);
}
