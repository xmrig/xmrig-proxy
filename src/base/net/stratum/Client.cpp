/* XMRig
 * Copyright (c) 2019      jtgrassie   <https://github.com/jtgrassie>
 * Copyright (c) 2018-2024 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include <cassert>
#include <cinttypes>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <sstream>


#ifdef XMRIG_FEATURE_TLS
#   include <openssl/ssl.h>
#   include <openssl/err.h>
#   include "base/net/stratum/Tls.h"
#endif


#include "base/net/stratum/Client.h"
#include "3rdparty/rapidjson/document.h"
#include "3rdparty/rapidjson/error/en.h"
#include "3rdparty/rapidjson/stringbuffer.h"
#include "3rdparty/rapidjson/writer.h"
#include "base/io/json/Json.h"
#include "base/io/json/JsonRequest.h"
#include "base/io/log/Log.h"
#include "base/kernel/constants.h"
#include "base/kernel/interfaces/IClientListener.h"
#include "base/kernel/Platform.h"
#include "base/net/dns/Dns.h"
#include "base/net/dns/DnsRecords.h"
#include "base/net/stratum/NativeTarget.h"
#include "base/net/stratum/Socks5.h"
#include "base/net/tools/NetBuffer.h"
#include "base/tools/Chrono.h"
#include "base/tools/cryptonote/BlobReader.h"
#include "base/tools/Cvt.h"
#include "net/JobResult.h"
#include "proxy/Miner.h"

#include <algorithm>


#ifdef _MSC_VER
#   define strncasecmp(x,y,z) _strnicmp(x,y,z)
#endif


namespace xmrig {

Storage<Client> Client::m_storage;
uint64_t Client::m_loginWindow      = 0;
uint64_t Client::m_getjobWindow     = 0;
uint8_t Client::m_loginWindowCount  = 0;
uint8_t Client::m_getjobWindowCount = 0;
std::deque<Client *> Client::m_getjobQueue;

} /* namespace xmrig */


namespace {

constexpr const char *kUnsupportedAlgoError = "algo array must include at least one supported pool algo:";

xmrig::CapabilityErrorLog g_capabilityErrorLog;
xmrig::CapabilityErrorLog g_poolErrorLog;

inline bool isUnsupportedAlgoError(const char *message)
{
    return message && strncmp(message, kUnsupportedAlgoError, strlen(kUnsupportedAlgoError)) == 0;
}

inline std::string capabilityErrorKey(const xmrig::Pool &pool, const char *message)
{
    std::string key(pool.host().data() ? pool.host().data() : "");
    key.push_back(':');
    key += std::to_string(pool.port());
    key.push_back('\n');
    key += message;

    return key;
}

inline std::string poolErrorKey(const xmrig::Pool &pool, const char *kind, const char *message)
{
    std::string key(kind ? kind : "error");
    key.push_back('\n');
    key += capabilityErrorKey(pool, message ? message : "unknown");

    return key;
}

inline bool isNoBlockTemplateError(const char *message)
{
    return message && strcmp(message, "No block template yet. Please wait.") == 0;
}

} /* namespace */


#ifdef APP_DEBUG
static const char *states[] = {
    "unconnected",
    "host-lookup",
    "connecting",
    "connected",
    "closing",
    "reconnecting"
};
#endif

xmrig::Client::Client(int id, const char *agent, IClientListener *listener) :
    BaseClient(id, listener),
    m_agent(agent),
    m_sendBuf(1024),
    m_tempBuf(320)
{
    m_reader.setListener(this);
    m_key = m_storage.add(this);
}


xmrig::Client::~Client()
{
    m_getjobQueue.erase(std::remove(m_getjobQueue.begin(), m_getjobQueue.end(), this), m_getjobQueue.end());
    delete m_socket;
}


bool xmrig::Client::disconnect()
{
    m_keepAlive = 0;
    m_expire    = 0;
    m_failures  = -1;

    return close();
}


bool xmrig::Client::isTLS() const
{
#   ifdef XMRIG_FEATURE_TLS
    return m_pool.isTLS() && m_tls;
#   else
    return false;
#   endif
}


const char *xmrig::Client::tlsFingerprint() const
{
#   ifdef XMRIG_FEATURE_TLS
    if (isTLS() && m_pool.fingerprint() == nullptr) {
        return m_tls->fingerprint();
    }
#   endif

    return nullptr;
}


const char *xmrig::Client::tlsVersion() const
{
#   ifdef XMRIG_FEATURE_TLS
    if (isTLS()) {
        return m_tls->version();
    }
#   endif

    return nullptr;
}


int64_t xmrig::Client::send(const rapidjson::Value &obj, Callback callback)
{
    assert(obj["id"] == sequence());

    m_callbacks.insert({ sequence(), std::move(callback) });

    return send(obj);
}


int64_t xmrig::Client::send(const rapidjson::Value &obj)
{
    return send(obj, false);
}


int64_t xmrig::Client::send(const rapidjson::Value &obj, bool pearl)
{
    if (m_state != ConnectedState || m_socket == nullptr || uv_is_writable(stream()) != 1) {
        return -1;
    }

    using namespace rapidjson;

    StringBuffer buffer(nullptr, 512);
    Writer<StringBuffer> writer(buffer);
    obj.Accept(writer);

    const size_t size = buffer.GetSize();
    const size_t maxSendBufferSize = pearl ? XMRIG_PEARL_MAX_FRAME_SIZE : kMaxSendBufferSize;
    if (size > maxSendBufferSize) {
        LOG_ERR("%s " RED("send failed: ") RED_BOLD("\"max send buffer size exceeded: %zu\""), tag(), size);
        close("send buffer limit");

        return -1;
    }

    if (size > (m_sendBuf.size() - 2)) {
        m_sendBuf.resize(((size + 1) / 1024 + 1) * 1024);
    }

    memcpy(m_sendBuf.data(), buffer.GetString(), size);
    m_sendBuf[size]     = '\n';
    m_sendBuf[size + 1] = '\0';

    if (pearl) {
        // Keep control messages usable while a large claim is still queued on this connection.
        m_maxWriteQueueSize = XMRIG_PEARL_MAX_WRITE_QUEUE_SIZE;
    }

    return send(size + 1);
}


int64_t xmrig::Client::submit(const JobResult &result)
{
    if (m_rpcId.isNull() || m_state != ConnectedState || m_socket == nullptr || uv_is_writable(stream()) != 1) {
        return -1;
    }

#   ifndef XMRIG_PROXY_PROJECT
    if (result.clientId != m_rpcId || m_rpcId.isNull() || m_state != ConnectedState) {
        return -1;
    }
#   endif

    if (result.diff == 0) {
        close("invalid share difficulty");

        return -1;
    }

    if (!result.nativePayload.isNull() && !result.nativePayload.isEmpty()) {
        return submitNative(result);
    }

    using namespace rapidjson;

#   ifdef XMRIG_PROXY_PROJECT
    const char *nonce = result.nonce;
    const char *data  = result.result;
#   else
    char *nonce = m_tempBuf.data();
    char *data  = m_tempBuf.data() + 16;
    char *signature = m_tempBuf.data() + 88;
    char *commitment = m_tempBuf.data() + 224;

    Cvt::toHex(nonce, sizeof(uint32_t) * 2 + 1, reinterpret_cast<const uint8_t *>(&result.nonce), sizeof(uint32_t));
    Cvt::toHex(data, 65, result.result(), 32);

    if (result.minerSignature()) {
        Cvt::toHex(signature, 129, result.minerSignature(), 64);
    }

    if (result.commitment()) {
        Cvt::toHex(commitment, 65, result.commitment(), 32);
    }
#   endif

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kObjectType);
    params.AddMember("id",     StringRef(m_rpcId.data()), allocator);
    params.AddMember("job_id", StringRef(result.jobId.data()), allocator);
    params.AddMember("nonce",  StringRef(nonce), allocator);
    params.AddMember("result", StringRef(data), allocator);

#   ifndef XMRIG_PROXY_PROJECT
    if (result.minerSignature()) {
        params.AddMember("sig", StringRef(signature), allocator);
    }
#   else
    if (result.sig) {
        params.AddMember("sig", StringRef(result.sig), allocator);
    }
#   endif

#   ifndef XMRIG_PROXY_PROJECT
    if (result.commitment()) {
        params.AddMember("commitment", StringRef(commitment), allocator);
    }
#   else
    if (result.commitment) {
        params.AddMember("commitment", StringRef(result.commitment), allocator);
    }
#   endif

    if (has<EXT_ALGO>() && result.algorithm.isValid()) {
        params.AddMember("algo", StringRef(result.algorithm.name()), allocator);
    }

    JsonRequest::create(doc, m_sequence, "submit", params);

#   ifdef XMRIG_PROXY_PROJECT
    m_results[m_sequence] = SubmitResult(m_sequence, result.diff, result.actualDiff(), result.id, 0);
#   else
    m_results[m_sequence] = SubmitResult(m_sequence, result.diff, result.actualDiff(), 0, result.backend);
#   endif
    m_results[m_sequence].assignedDiff = result.assignedDiff;
    m_results[m_sequence].minerIp = result.minerIp;

    return send(doc);
}


void xmrig::Client::connect()
{
    clearLogCloseReason();

    if (m_pool.proxy().isValid()) {
        m_socks5 = new Socks5(this);
        resolve(m_pool.proxy().host());

        return;
    }

#   ifdef XMRIG_FEATURE_TLS
    if (m_pool.isTLS()) {
        m_tls = new Tls(this);
    }
#   endif

    resolve(m_pool.host());
}


void xmrig::Client::connect(const Pool &pool)
{
    setPool(pool);
    connect();
}


void xmrig::Client::deleteLater()
{
    if (!m_listener) {
        return;
    }

    m_listener = nullptr;

    if (!disconnect()) {
        m_storage.remove(m_key);
    }
}


void xmrig::Client::tick(uint64_t now)
{
    if (m_state == ConnectedState) {
        if (m_loginPending && !m_loginInFlight) {
            login();
        }

        if (m_getjobDirty && !m_getjobInFlight) {
            sendGetjob();
        }

        if (m_expire && now > m_expire) {
            LOG_DEBUG_ERR("[%s] timeout", url());
            close("response timeout");
        }
        else if (m_keepAlive && now > m_keepAlive) {
            ping();
        }

        return;
    }

    if (m_state == ReconnectingState && m_expire && now > m_expire) {
        return connect();
    }

    if (m_state == ConnectingState && m_expire && now > m_expire) {
        close("connect timeout");
    }
}


/* MoneroOcean change: begin These wrappers keep miner algo switching on concrete normal stratum clients instead of adding MoneroOcean methods to IClient. */
bool xmrig::Client::tryMiner(const Miner *miner, int upstreamCount) const
{
    return AlgoSwitch::tryMiner(miner, upstreamCount);
}


bool xmrig::Client::requiresPearlLogin(const Miner *miner) const
{
    return AlgoSwitch::requiresPearlLogin(miner);
}


void xmrig::Client::addMiner(const Miner *miner)
{
    AlgoSwitch::addMiner(miner);
    if (miner && miner->hasExtension(Miner::EXT_NATIVE)) {
        m_nativeRequested = true;
        subscribeNative();
    }
    getjob();
}


void xmrig::Client::removeMiner(const Miner *miner)
{
    AlgoSwitch::removeMiner(miner);
    getjob();
}


void xmrig::Client::setAlgoPerfSameThreshold(uint64_t percent)
{
    AlgoSwitch::setSameThreshold(percent);
}


void xmrig::Client::setPool(const Pool &pool)
{
    BaseClient::setPool(pool);
    AlgoSwitch::setDefaultAlgo(pool.algorithm());

    m_nativeControl   = nullptr;
    m_nativeControlAlgo = nullptr;
    m_nativePrefix    = nullptr;
    m_nativeTarget    = nullptr;
    m_nativeNonceSize = 0;
    m_nativeSubscribed = false;
    m_nativePrefixUpdated = false;
    m_logOffered.clear();
    clearLogCloseReason();
    m_getjobAlgos.clear();
    m_getjobCooldown.clear();
}
/* MoneroOcean change: end */


void xmrig::Client::onResolved(const DnsRecords &records, int status, const char *error)
{
    m_dns.reset();

    assert(m_listener != nullptr);
    if (!m_listener) {
        return reconnect();
    }

    if (status < 0 && records.isEmpty()) {
        if (!isQuiet()) {
            LOG_ERR("%s " RED("DNS error: ") RED_BOLD("\"%s\""), tag(), error);
        }

        return reconnect();
    }

    const auto &record = records.get();
    m_ip = record.ip();

    connect(record.addr(m_socks5 ? m_pool.proxy().port() : m_pool.port()));
}


bool xmrig::Client::close(const char *reason)
{
    if (m_state == ClosingState) {
        return m_socket != nullptr;
    }

    if (m_state == UnconnectedState || m_socket == nullptr) {
        return false;
    }

    setLogCloseReason(reason);

    m_rpcId = nullptr;
    m_loginPending   = false;
    m_loginInFlight  = false;
    m_getjobDirty    = false;
    m_getjobInFlight = false;
    m_getjobAlgos.clear();
    m_getjobCooldown.clear();

    setState(ClosingState);

    if (uv_is_closing(reinterpret_cast<uv_handle_t*>(m_socket)) == 0) {
        if (Platform::hasKeepalive()) {
            uv_tcp_keepalive(m_socket, 0, 60);
        }
        uv_close(reinterpret_cast<uv_handle_t*>(m_socket), Client::onClose);
    }

    return true;
}


bool xmrig::Client::parseJob(const rapidjson::Value &params, int *code, const rapidjson::Value *payload)
{
    if (!params.IsObject()) {
        *code = 2;
        return false;
    }

    Job job(has<EXT_NICEHASH>(), m_pool.algorithm(), m_rpcId);

    if (!job.setId(Json::getString(params, "job_id"))) {
        *code = 3;
        return false;
    }

    const char *algo = Json::getString(params, "algo");
    const char *blobData = Json::getString(params, "blob");
    if (algo) {
        job.setAlgorithm(algo);
    }
    else if (m_pool.coin().isValid()) {
        uint8_t blobVersion = 0;
        if (blobData) {
            Cvt::fromHex(&blobVersion, 1, blobData, 2);
        }
        job.setAlgorithm(m_pool.coin().algorithm(blobVersion));
    }
    if (!blobData && job.algorithm() == Algorithm::PEARLHASH) {
        blobData = Json::getString(params, "header");
    }

#   ifdef XMRIG_PROXY_PROJECT
    if (!m_nativePrefix.isNull()) {
        job.setNativePrefix(m_nativePrefix.data());
    }

    if (m_nativeRequested && job.algorithm() == Algorithm::C29 && params.HasMember("noncebytes")) {
        const uint64_t nonceBytes = Json::getUint64(params, "noncebytes");
        const uint64_t nonceOffset = Json::getUint64(params, "nonceoffset");
        if (nonceBytes > 0 && nonceBytes <= 8 && nonceOffset <= Job::kMaxBlobSize - nonceBytes) {
            job.setNativeNonce(static_cast<size_t>(nonceOffset), static_cast<size_t>(nonceBytes));
        }
    }
#   endif

#   ifdef XMRIG_FEATURE_HTTP
    if (m_pool.mode() == Pool::MODE_SELF_SELECT) {
        job.setExtraNonce(Json::getString(params, "extra_nonce"));
        job.setPoolWallet(Json::getString(params, "pool_wallet"));

        if (job.extraNonce().isNull() || job.poolWallet().isNull()) {
            *code = 4;
            return false;
        }
    }
    else
#   endif
    {
        if (!job.setBlob(blobData)) {
            if (!m_nativeRequested || job.algorithm() != Algorithm::C29) {
                *code = 4;
                return false;
            }

            const char *prePow = Json::getString(params, "pre_pow");
            std::string synthetic;
            if (!prePow && job.algorithm() == Algorithm::C29) {
                synthetic.assign((job.nonceOffset() + job.nonceSize()) * 2, '0');
                prePow = synthetic.c_str();
            }

            if (!job.setBlob(prePow)) {
                *code = 4;
                return false;
            }
        }
    }

    const char *target = Json::getString(params, "target");
    if (!job.setTarget(target)) {
#   ifdef XMRIG_PROXY_PROJECT
        if (job.algorithm() == Algorithm::PEARLHASH) {
            const uint64_t difficulty = NativeTarget::difficulty(target, true);
            if (!difficulty || !job.setNativeTarget(target, true)) {
                *code = 5;
                return false;
            }
            job.setDiff(difficulty);
        }
        else
#   endif
        {
            if (!m_nativeRequested || job.algorithm() != Algorithm::C29) {
                *code = 5;
                return false;
            }

            const uint64_t difficulty = Json::getUint64(params, "difficulty");
            if (difficulty == 0) {
                *code = 5;
                return false;
            }

            job.setDiff(difficulty);
        }
    }

    job.setHeight(Json::getUint64(params, "height"));

    if (!verifyAlgorithm(job.algorithm(), algo)) {
        *code = 6;
        return false;
    }

    if (m_pool.mode() != Pool::MODE_SELF_SELECT && job.algorithm().family() == Algorithm::RANDOM_X && !job.setSeedHash(Json::getString(params, "seed_hash"))) {
        *code = 7;
        return false;
    }

    job.setSigKey(Json::getString(params, "sig_key"));

#   ifdef XMRIG_PROXY_PROJECT
    if (!payload) {
        payload = m_currentMessage;
    }
    if (payload && payload->IsObject() && payload->HasMember("method")) {
        job.setNativePayload(*payload);
    }
    else if (Algorithm::isNativeOnly(job.algorithm().id())) {
        rapidjson::Document nativePayload(rapidjson::kObjectType);
        auto &allocator = nativePayload.GetAllocator();
        nativePayload.AddMember("method", rapidjson::Value("job", allocator), allocator);
        rapidjson::Value nativeParams(rapidjson::kObjectType);
        nativeParams.CopyFrom(params, allocator);
        nativePayload.AddMember("params", nativeParams, allocator);
        job.setNativePayload(nativePayload);
    }
#   endif

    m_job.setClientId(m_rpcId);

    if (m_job != job) {
        m_nativePrefixUpdated = false;
        m_jobs++;
        m_job = std::move(job);
        return true;
    }

    if (m_nativePrefixUpdated) {
        m_nativePrefixUpdated = false;
        return false;
    }

    if (m_jobs == 0) { // https://github.com/xmrig/xmrig/issues/459
        return false;
    }

    if (!isQuiet()) {
        LOG_WARN("%s " YELLOW("duplicate job received, reconnect"), tag());
    }

    close("duplicate job");
    return false;
}


bool xmrig::Client::send(BIO *bio)
{
#   ifdef XMRIG_FEATURE_TLS
    uv_buf_t buf;
    buf.len = BIO_get_mem_data(bio, &buf.base); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

    if (buf.len == 0) {
        return true;
    }

    LOG_DEBUG("[%s] TLS send     (%d bytes)", url(), static_cast<int>(buf.len));

    bool result = false;
    if (state() == ConnectedState && m_socket != nullptr && uv_is_writable(stream())) {
        result = write(buf);
    }
    else {
        LOG_DEBUG_ERR("[%s] send failed, invalid state: %d", url(), m_state);
    }

    (void) BIO_reset(bio);

    return result;
#   else
    return false;
#   endif
}


bool xmrig::Client::verifyAlgorithm(const Algorithm &algorithm, const char *algo) const
{
    if (!algorithm.isValid()) {
        if (!isQuiet()) {
            if (algo == nullptr) {
                LOG_ERR("%s " RED("unknown algorithm, make sure you set \"algo\" or \"coin\" option"), tag(), algo);
            }
            else {
                LOG_ERR("%s " RED("unsupported algorithm ") RED_BOLD("\"%s\" ") RED("detected, reconnect"), tag(), algo);
            }
        }

        return false;
    }

    bool ok = true;
    m_listener->onVerifyAlgorithm(this, algorithm, &ok);

    if (!ok && !isQuiet()) {
        LOG_ERR("%s " RED("incompatible/disabled algorithm ") RED_BOLD("\"%s\" ") RED("detected, reconnect"), tag(), algorithm.name());
    }

    return ok;
}


bool xmrig::Client::write(const uv_buf_t &buf)
{
    if (buf.len > m_maxWriteQueueSize ||
        uv_stream_get_write_queue_size(stream()) > m_maxWriteQueueSize - buf.len) {
        if (!isQuiet()) {
            LOG_ERR("%s " RED("write error: ") RED_BOLD("\"write queue limit exceeded\""), tag());
        }
        close("write queue limit");
        return false;
    }

    struct WriteRequest {
        uv_write_t request{};
        std::vector<char> data;
    };
    auto *request = new WriteRequest;
    request->data.assign(buf.base, buf.base + buf.len);
    request->request.data = request;
    uv_buf_t copy = uv_buf_init(request->data.data(), static_cast<unsigned int>(request->data.size()));
    const int rc = uv_write(&request->request, stream(), &copy, 1, [](uv_write_t *req, int) {
        delete static_cast<WriteRequest *>(req->data);
    });
    if (rc == 0) {
        return true;
    }

    delete request;
    if (!isQuiet()) {
        LOG_ERR("%s " RED("write error: ") RED_BOLD("\"%s\""), tag(), uv_strerror(rc));
    }
    close(uv_strerror(rc));

    return false;
}


int xmrig::Client::resolve(const String &host)
{
    setState(HostLookupState);

    m_reader.reset();

    if (m_failures == -1) {
        m_failures = 0;
    }

    m_dns = Dns::resolve(host, this);

    return 0;
}


int64_t xmrig::Client::send(size_t size)
{
    LOG_DEBUG("[%s] send (%d bytes): \"%.*s\"", url(), size, static_cast<int>(size) - 1, m_sendBuf.data());

#   ifdef XMRIG_FEATURE_TLS
    if (isTLS()) {
        if (!m_tls->send(m_sendBuf.data(), size)) {
            return -1;
        }
    }
    else
#   endif
    {
        if (state() != ConnectedState || m_socket == nullptr || !uv_is_writable(stream())) {
            LOG_DEBUG_ERR("[%s] send failed, invalid state: %d", url(), m_state);
            return -1;
        }

        uv_buf_t buf = uv_buf_init(m_sendBuf.data(), (unsigned int) size);

        if (!write(buf)) {
            return -1;
        }
    }

    m_expire = Chrono::steadyMSecs() + kResponseTimeout;
    startTimeout();
    return m_sequence++;
}


void xmrig::Client::connect(const sockaddr *addr)
{
    setState(ConnectingState);

    auto req = new uv_connect_t;
    req->data = m_storage.ptr(m_key);

    m_socket = new uv_tcp_t;
    m_socket->data = m_storage.ptr(m_key);

    uv_tcp_init(uv_default_loop(), m_socket);
    uv_tcp_nodelay(m_socket, 1);

    if (Platform::hasKeepalive()) {
        uv_tcp_keepalive(m_socket, 1, 60);
    }

    const int status = uv_tcp_connect(req, m_socket, addr, onConnect);
    if (status < 0) {
        // Immediate failures do not invoke the callback or release the request.
        onConnect(req, status);
    }
}


void xmrig::Client::handshake()
{
    if (m_socks5) {
        return m_socks5->handshake();
    }

#   ifdef XMRIG_FEATURE_TLS
    if (isTLS()) {
        m_expire = Chrono::steadyMSecs() + kResponseTimeout;

        m_tls->handshake(m_pool.isSNI() ? m_pool.host().data() : nullptr);
    }
    else
#   endif
    {
        login();
    }
}


bool xmrig::Client::parseLogin(const rapidjson::Value &result, int *code)
{
    setRpcId(Json::getString(result, "id"));
    if (rpcId().isNull()) {
        *code = 1;
        return false;
    }

    parseExtensions(result);
    setNativeMetadata(result, false);

    const rapidjson::Value &job = Json::getObject(result, "job");
    bool rc = true;
    if (job.IsObject()) {
        rc = parseJob(job, code);
    }
    else if (result.HasMember("job_id")) {
        rc = parseJob(result, code);
    }
    m_jobs = 0;

    return rc;
}


/* MoneroOcean change: begin getjob returns a top-level job object, so parsing it separately avoids treating refreshed jobs as new logins. */
bool xmrig::Client::parseGetjob(const rapidjson::Value &result, int *code)
{
    setRpcId(Json::getString(result, "id"));
    if (rpcId().isNull()) {
        *code = 1;
        return false;
    }

    parseExtensions(result);
    setNativeMetadata(result, true);

    const rapidjson::Value &job = Json::getObject(result, "job");
    if (job.IsObject()) {
        return parseJob(job, code);
    }

    if (result.HasMember("job_id")) {
        return parseJob(result, code);
    }

    return true;
}
/* MoneroOcean change: end */


void xmrig::Client::login()
{
    if (m_state != ConnectedState || m_socket == nullptr || uv_is_writable(stream()) != 1) {
        m_loginPending = true;
        m_expire = 0;

        return;
    }

    if (m_loginInFlight) {
        return;
    }

    const uint64_t now = Chrono::steadyMSecs();
    if (m_loginWindow == 0 || now - m_loginWindow >= kUpstreamRequestWindow) {
        m_loginWindow = now;
        m_loginWindowCount = 0;
    }

    if (m_loginWindowCount >= kUpstreamRequestsPerWindow) {
        m_loginPending = true;
        m_expire = 0;

        return;
    }

    ++m_loginWindowCount;
    m_loginPending = false;

    using namespace rapidjson;
    m_results.clear();

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kObjectType);
    params.AddMember("login", m_user.toJSON(),     allocator);
    params.AddMember("pass",  m_password.toJSON(), allocator);
    params.AddMember("agent", StringRef(m_agent),  allocator);
    /* MoneroOcean change: begin Advertise normalized miner algo/algo-perf capabilities so MoneroOcean can choose compatible work. */
    Value algos = AlgoSwitch::algosToJSON(doc);
    m_pearlLogin = std::any_of(algos.Begin(), algos.End(), [](const Value &algo) {
        return algo.IsString() && strcmp(algo.GetString(), Algorithm::kPEARLHASH) == 0;
    });
    params.AddMember("algo", algos, allocator);
    params.AddMember("algo-perf", AlgoSwitch::algoPerfsToJSON(doc), allocator);
    Value extensions(kArrayType);
    extensions.PushBack("mo-native", allocator);
    extensions.PushBack("submit-result", allocator);
    params.AddMember("extensions", extensions, allocator);
    /* MoneroOcean change: end */

    if (!m_rigId.isNull()) {
        params.AddMember("rigid", m_rigId.toJSON(), allocator);
    }

    m_listener->onLogin(this, doc, params);

    JsonRequest::create(doc, 1, "login", params);

    if (send(doc) < 0) {
        m_loginPending = true;

        return;
    }

    captureLogOffered(Json::getValue(doc, "params"));
    // This request includes current capabilities; later miner changes still need getjob.
    m_getjobDirty = false;
    m_loginInFlight = true;
}


/* MoneroOcean change: begin Refresh the upstream job when miner capabilities change instead of reconnecting the pool client. */
void xmrig::Client::getjob()
{
    m_getjobDirty = true;
    sendGetjob();
}


void xmrig::Client::sendGetjob()
{
    if (!m_getjobDirty) {
        return;
    }

    // Coalesce each group's changes and share the existing budget fairly under miner churn.
    if (std::find(m_getjobQueue.begin(), m_getjobQueue.end(), this) == m_getjobQueue.end()) {
        m_getjobQueue.push_back(this);
    }

    const uint64_t now = Chrono::steadyMSecs();
    if (m_getjobWindow == 0 || now - m_getjobWindow >= kUpstreamRequestWindow) {
        m_getjobWindow = now;
        m_getjobWindowCount = 0;
    }

    while (!m_getjobQueue.empty() && m_getjobWindowCount < kUpstreamRequestsPerWindow) {
        Client *client = m_getjobQueue.front();
        m_getjobQueue.pop_front();
        client->sendGetjobRequest();
    }
}


void xmrig::Client::sendGetjobRequest()
{
    using namespace rapidjson;

    if (!m_getjobDirty || m_getjobInFlight || !m_rpcId || m_state != ConnectedState || m_socket == nullptr || uv_is_writable(stream()) != 1) {
        return;
    }

    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();
    Value algos = AlgoSwitch::algosToJSON(doc);
    StringBuffer algoBuffer(nullptr, 256);
    Writer<StringBuffer> algoWriter(algoBuffer);
    algos.Accept(algoWriter);
    std::string sentAlgos(algoBuffer.GetString(), algoBuffer.GetSize());

    const uint64_t now = Chrono::steadyMSecs();
    if (!m_getjobCooldown.allows(now, sentAlgos)) {
        return;
    }

    ++m_getjobWindowCount;
    m_getjobDirty = false;

    Value params(kObjectType);
    params.AddMember("id", StringRef(m_rpcId.data()), allocator);
    params.AddMember("algo", algos, allocator);
    params.AddMember("algo-perf", AlgoSwitch::algoPerfsToJSON(doc), allocator);

    JsonRequest::create(doc, 1, "getjob", params);

    if (send(doc) < 0) {
        m_getjobDirty = true;

        return;
    }

    captureLogOffered(Json::getValue(doc, "params"));
    m_getjobAlgos = std::move(sentAlgos);
    m_getjobInFlight = true;
}
/* MoneroOcean change: end */

void xmrig::Client::onClose()
{
    m_maxWriteQueueSize = kMaxSendBufferSize;
    delete m_socket;

    m_socket = nullptr;
    setState(UnconnectedState);

#   ifdef XMRIG_FEATURE_TLS
    if (m_tls) {
        delete m_tls;
        m_tls = nullptr;
    }
#   endif

    m_nativeSubscribed = false;
    m_nativePrefix = nullptr;
    m_nativeControl = nullptr;
    m_nativeControlAlgo = nullptr;
    m_nativeTarget = nullptr;
    m_nativeNonceSize = 0;
    m_nativePrefixUpdated = false;
    m_currentMessage = nullptr;
    m_getjobAlgos.clear();
    m_getjobCooldown.clear();
    m_job.reset();
    m_jobs = 0;
    m_rpcId = nullptr;

    reconnect();
}


void xmrig::Client::parse(char *line, size_t len)
{
    LOG_DEBUG("[%s] received (%d bytes): \"%.*s\"", url(), len, static_cast<int>(len), line);

    if (len < 22 || line[0] != '{') {
        if (!isQuiet()) {
            LOG_ERR("%s " RED("JSON decode failed"), tag());
        }

        return;
    }

    rapidjson::Document doc;
    if (doc.ParseInsitu(line).HasParseError()) {
        if (!isQuiet()) {
            LOG_ERR("%s " RED("JSON decode failed: ") RED_BOLD("\"%s\""), tag(), rapidjson::GetParseError_En(doc.GetParseError()));
        }

        return;
    }

    if (!doc.IsObject()) {
        return;
    }

    m_currentMessage = &doc;

    const auto &id    = Json::getValue(doc, "id");
    const auto &error = Json::getValue(doc, "error");
    const char *method = Json::getString(doc, "method");

    if (method && strcmp(method, "client.reconnect") == 0) {
        const auto &params = Json::getValue(doc, "params");
        if (!params.IsArray()) {
            LOG_ERR("%s " RED("invalid client.reconnect notification: params is not an array"), tag());
            return;
        }

        auto arr = params.GetArray();

        if (arr.Empty()) {
            LOG_ERR("%s " RED("invalid client.reconnect notification: params array is empty"), tag());
            return;
        }

        if (arr.Size() != 2) {
            LOG_ERR("%s " RED("invalid client.reconnect notification: params array has wrong size"), tag());
            return;
        }

        if (!arr[0].IsString()) {
            LOG_ERR("%s " RED("invalid client.reconnect notification: host is not a string"), tag());
            return;
        }

        if (!arr[1].IsString()) {
            LOG_ERR("%s " RED("invalid client.reconnect notification: port is not a string"), tag());
            return;
        }

        std::stringstream s;
        s << arr[0].GetString() << ":" << arr[1].GetString();
        LOG_WARN("%s " YELLOW("client.reconnect to %s"), tag(), s.str().c_str());
        setPoolUrl(s.str().c_str());
        return reconnect();
    }

    if (m_nativeRequested && method && strcmp(method, "mining.set_target") == 0) {
        parseNativeControl(doc);
        return;
    }

    if (m_nativeRequested && method && strcmp(method, "mining.set_difficulty") == 0) {
        parseNativeControl(doc);
        return;
    }

    if (m_nativeRequested && method && strcmp(method, "mining.set_extranonce") == 0) {
        parseNativeControl(doc);
        return;
    }

    if ((m_nativeRequested || m_pool.algorithm() == Algorithm::PEARLHASH) && method &&
        strcmp(method, "mining.notify") == 0) {
        if (parseNativeNotify(doc)) {
            m_listener->onJobReceived(this, m_job, doc);
        }
        else {
            close("invalid native notify");
        }

        return;
    }

    if (method && strcmp(method, "job") == 0 && m_nativeRequested) {
        const auto &params = Json::getValue(doc, "params");
        const bool nativeObject = params.IsObject() &&
                                  (params.HasMember("noncebytes") || params.HasMember("pre_pow") ||
                                   params.HasMember("proof") || params.HasMember("edges"));
        if (nativeObject) {
            if (parseNativeObjectJob(doc)) {
                m_listener->onJobReceived(this, m_job, doc);
            }
            else {
                close("invalid native job");
            }

            return;
        }
    }

    if (id.IsInt64()) {
        return parseResponse(id.GetInt64(), Json::getValue(doc, "result"), error);
    }

    if (!method) {
        return;
    }

    if (error.IsObject()) {
        if (!isQuiet()) {
            LOG_ERR("%s " RED("error: ") RED_BOLD("\"%s\"") RED(", code: ") RED_BOLD("%d"),
                    tag(), logText(Json::getString(error, "message")).c_str(), Json::getInt(error, "code"));
        }

        return;
    }

    parseNotification(method, Json::getValue(doc, "params"), error);
}


void xmrig::Client::parseExtensions(const rapidjson::Value &result)
{
    m_extensions.reset();

    if (!result.HasMember("extensions")) {
        return;
    }

    const rapidjson::Value &extensions = result["extensions"];
    if (!extensions.IsArray()) {
        return;
    }

    for (const rapidjson::Value &ext : extensions.GetArray()) {
        if (!ext.IsString()) {
            continue;
        }

        const char *name = ext.GetString();

        if (strcmp(name, "algo") == 0) {
            setExtension(EXT_ALGO, true);
        }
        else if (strcmp(name, "nicehash") == 0) {
            setExtension(EXT_NICEHASH, true);
        }
        else if (strcmp(name, "connect") == 0) {
            setExtension(EXT_CONNECT, true);
        }
        else if (strcmp(name, "keepalive") == 0) {
            setExtension(EXT_KEEPALIVE, true);
            startTimeout();
        }
#       ifdef XMRIG_FEATURE_TLS
        else if (strcmp(name, "tls") == 0) {
            setExtension(EXT_TLS, true);
        }
#       endif
    }
}


void xmrig::Client::parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &)
{
    if (strcmp(method, "job") == 0) {
        int code = -1;
        if (parseJob(params, &code)) {
            m_listener->onJobReceived(this, m_job, params);
        }
        else {
            close("invalid job notification");
        }

        return;
    }
}


void xmrig::Client::parseResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error)
{
    if (handleResponse(id, result, error)) {
        return;
    }

    if (error.IsObject()) {
        const char *message = error["message"].GetString();
        const bool getjob = id == 1 && m_getjobInFlight;

        if (id == 1) {
            if (getjob) {
                m_getjobInFlight = false;
                if (isUnsupportedAlgoError(message)) {
                    m_getjobDirty = true;
                    m_getjobCooldown.reject(Chrono::steadyMSecs(), m_getjobAlgos);
                }
                else {
                    m_getjobDirty = false;
                    m_getjobCooldown.clear();
                }
            }
            else {
                m_loginInFlight = false;
            }
        }

        if (!handleSubmitResponse(id, message) && !isQuiet()) {
            const bool duplicateCapabilityError = id == 1
                && isUnsupportedAlgoError(message)
                && !g_capabilityErrorLog.allows(Chrono::steadyMSecs(), capabilityErrorKey(m_pool, message));
            const bool duplicatePoolError = id == 1
                && isNoBlockTemplateError(message)
                && !g_poolErrorLog.allows(Chrono::steadyMSecs(), poolErrorKey(m_pool, "response", message));

            if (!duplicateCapabilityError && !duplicatePoolError) {
                LOG_ERR("%s " RED("error: ") RED_BOLD("\"%s\"") RED(", code: ") RED_BOLD("%d"), tag(), logText(message).c_str(), Json::getInt(error, "code"));
            }
        }

        /* A getjob rejection is not a failed login.  In particular, do not force
         * the backup strategy to reconnect for a pool-side getjob rate limit. */
        if ((!getjob && m_id == 1) || isCriticalError(message)) {
            close(message);
        }

        return;
    }

    if (id != 1 && !result.IsObject()) {
        if (!result.IsBool()) {
            handleSubmitResponse(id, "invalid response");
        }
        else if (result.GetBool()) {
            handleSubmitResponse(id);
        }
        else {
            handleSubmitResponse(id, "pool rejected share");
        }

        return;
    }

    if (id == 1) {
        if (m_getjobInFlight && result.IsNull()) {
            m_getjobInFlight = false;
            m_getjobCooldown.clear();
            if (m_getjobDirty) {
                sendGetjob();
            }

            return;
        }

        // Pearl's login dialect acknowledges authorization with a boolean and sends work in a
        // following object-form mining.notify. Keep an internal client ID; Pearl submits do not
        // place it on the wire.
        if (m_loginInFlight && m_pearlLogin && result.IsBool()) {
            m_loginInFlight = false;
            if (!result.GetBool()) {
                close("login rejected");
                return;
            }

            setRpcId("pearlhash");
            m_failures = 0;
            m_getjobInFlight = false;
            m_listener->onLoginSuccess(this);
            return;
        }

        if (!result.IsObject()) {
            return;
        }

        int code = -1;
        if (m_getjobInFlight) {
            m_getjobInFlight = false;
            if (parseGetjob(result, &code)) {
                m_getjobCooldown.clear();
                const rapidjson::Value &job = Json::getObject(result, "job");
                if (m_job.isValid() && (job.IsObject() || result.HasMember("job_id"))) {
                    m_listener->onJobReceived(this, m_job, job.IsObject() ? job : result);
                }
            }
            else if (code != -1) {
                if (!isQuiet()) {
                    LOG_ERR("%s " RED("getjob error code: ") RED_BOLD("%d"), tag(), code);
                }
                close("invalid getjob response");
            }

            if (m_getjobDirty) {
                sendGetjob();
            }
            return;
        }

        m_loginInFlight = false;
        if (!parseLogin(result, &code)) {
            if (!isQuiet()) {
                LOG_ERR("%s " RED("login error code: ") RED_BOLD("%d"), tag(), code);
            }

            close("invalid login response");
            return;
        }

        m_failures = 0;
        // Preserve capability changes made while the login reply was in flight.
        m_getjobInFlight = false;
        m_listener->onLoginSuccess(this);

        subscribeNative();

        if (m_job.isValid()) {
            const rapidjson::Value &job = Json::getObject(result, "job");
            m_listener->onJobReceived(this, m_job, job.IsObject() ? job : result);
        }

        sendGetjob();
        return;
    }

    handleSubmitResponse(id);
}


void xmrig::Client::ping()
{
    send(snprintf(m_sendBuf.data(), m_sendBuf.size(), "{\"id\":%" PRId64 ",\"jsonrpc\":\"2.0\",\"method\":\"keepalived\",\"params\":{\"id\":\"%s\"}}\n", m_sequence, m_rpcId.data()));
}


void xmrig::Client::read(ssize_t nread, const uv_buf_t *buf)
{
    const auto size = static_cast<size_t>(nread);
    if (nread < 0) {
        const char *error = uv_strerror(static_cast<int>(nread));
        const bool duplicateEof = strcmp(error, "end of file") == 0
            && !g_poolErrorLog.allows(Chrono::steadyMSecs(), poolErrorKey(m_pool, "read", error));

        if (!isQuiet() && !duplicateEof) {
            LOG_ERR("%s " RED("read error: ") RED_BOLD("\"%s\""), tag(), error);
        }

        close(error);
        return;
    }

    assert(m_listener != nullptr);
    if (!m_listener) {
        return reconnect();
    }

    if (m_socks5) {
        m_socks5->read(buf->base, size);

        if (m_socks5->isReady()) {
            delete m_socks5;
            m_socks5 = nullptr;

#           ifdef XMRIG_FEATURE_TLS
            if (m_pool.isTLS() && !m_tls) {
                m_tls = new Tls(this);
            }
#           endif

            handshake();
        }

        return;
    }

#   ifdef XMRIG_FEATURE_TLS
    if (isTLS()) {
        LOG_DEBUG("[%s] TLS received (%d bytes)", url(), static_cast<int>(nread));

        m_tls->read(buf->base, size);
    }
    else
#   endif
    {
        m_reader.parse(buf->base, size);
    }
}


void xmrig::Client::reconnect()
{
    if (!m_listener) {
        m_storage.remove(m_key);

        return;
    }

    m_keepAlive = 0;

    if (m_failures == -1) {
        return m_listener->onClose(this, -1);
    }

    setState(ReconnectingState);

    m_failures++;
    m_listener->onClose(this, static_cast<int>(m_failures));
}


void xmrig::Client::setState(SocketState state)
{
    LOG_DEBUG("[%s] state: \"%s\" -> \"%s\"", url(), states[m_state], states[state]);

    if (m_state == state) {
        return;
    }

    switch (state) {
    case HostLookupState:
        m_expire = 0;
        break;

    case ConnectingState:
        m_expire = Chrono::steadyMSecs() + kConnectTimeout;
        break;

    case ReconnectingState:
        m_expire = Chrono::steadyMSecs() + m_retryPause;
        break;

    default:
        break;
    }

    m_state = state;
}


void xmrig::Client::startTimeout()
{
    m_expire = 0;

    if (has<EXT_KEEPALIVE>()) {
        const uint64_t ms = static_cast<uint64_t>(m_pool.keepAlive() > 0 ? m_pool.keepAlive() : Pool::kKeepAliveTimeout) * 1000;

        m_keepAlive = Chrono::steadyMSecs() + ms;
    }
}


bool xmrig::Client::isCriticalError(const char *message)
{
    if (!message) {
        return false;
    }

    if (strncasecmp(message, "Unauthenticated", 15) == 0) {
        return true;
    }

    if (strncasecmp(message, "your IP is banned", 17) == 0) {
        return true;
    }

    if (strncasecmp(message, "IP Address currently banned", 27) == 0) {
        return true;
    }

    if (strncasecmp(message, "Invalid job id", 14) == 0) {
        return true;
    }

    return false;
}


void xmrig::Client::onClose(uv_handle_t *handle)
{
    auto client = getClient(handle->data);
    if (!client) {
        return;
    }

    client->onClose();
}


void xmrig::Client::onConnect(uv_connect_t *req, int status)
{
    auto client = getClient(req->data);
    delete req;

    if (!client) {
        return;
    }

    if (status < 0) {
        if (!client->isQuiet()) {
            LOG_ERR("%s %s " RED("connect error: ") RED_BOLD("\"%s\""), client->tag(), client->ip().data(), uv_strerror(status));
        }

        if (client->state() == ReconnectingState || client->state() == ClosingState) {
            return;
        }

        if (client->state() != ConnectingState) {
            return;
        }

        client->close(uv_strerror(status));
        return;
    }

    if (client->state() == ConnectedState) {
        return;
    }

    client->setState(ConnectedState);

    uv_read_start(client->stream(), NetBuffer::onAlloc, onRead);

    client->handshake();
}


void xmrig::Client::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    auto client = getClient(stream->data);
    if (client) {
        client->read(nread, buf);
    }

    NetBuffer::release(buf);
}
