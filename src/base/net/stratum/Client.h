/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_CLIENT_H
#define XMRIG_CLIENT_H


#include <bitset>
#include <map>
#include <uv.h>
#include <vector>


#include "base/kernel/interfaces/IDnsListener.h"
#include "base/kernel/interfaces/ILineListener.h"
#include "base/net/stratum/Job.h"
#include "base/net/stratum/Pool.h"
#include "base/net/stratum/SubmitResult.h"
#include "base/net/tools/RecvBuf.h"
#include "base/net/tools/Storage.h"
#include "common/crypto/Algorithm.h"



typedef struct bio_st BIO;


namespace xmrig {


class IClientListener;
class JobResult;


class Client : public IDnsListener, public ILineListener
{
public:
    enum SocketState {
        UnconnectedState,
        HostLookupState,
        ConnectingState,
        ConnectedState,
        ClosingState
    };

    enum Extension {
        EXT_ALGO,
        EXT_NICEHASH,
        EXT_CONNECT,
        EXT_TLS,
        EXT_KEEPALIVE,
        EXT_MAX
    };

    constexpr static int kResponseTimeout = 20 * 1000;

#   ifdef XMRIG_FEATURE_TLS
    constexpr static int kInputBufferSize = 1024 * 16;
#   else
    constexpr static int kInputBufferSize = 1024 * 2;
#   endif

    Client(int id, const char *agent, IClientListener *listener);
    ~Client() override;

    bool disconnect();
    bool isTLS() const;
    const char *tlsFingerprint() const;
    const char *tlsVersion() const;
    int64_t submit(const JobResult &result);
    void connect();
    void connect(const Pool &pool);
    void deleteLater();
    void setPool(const Pool &pool);
    void tick(uint64_t now);

    inline bool isEnabled() const                     { return m_enabled; }
    inline bool isReady() const                       { return m_state == ConnectedState && m_failures == 0; }
    inline const char *host() const                   { return m_pool.host(); }
    inline const char *ip() const                     { return m_ip; }
    inline const Job &job() const                     { return m_job; }
    inline const Pool &pool() const                   { return m_pool; }
    inline int id() const                             { return m_id; }
    inline SocketState state() const                  { return m_state; }
    inline uint16_t port() const                      { return m_pool.port(); }
    inline void setAlgo(const Algorithm &algo)        { m_pool.setAlgo(algo); }
    inline void setEnabled(bool enabled)              { m_enabled = enabled; }
    inline void setQuiet(bool quiet)                  { m_quiet = quiet; }
    inline void setRetries(int retries)               { m_retries = retries; }
    inline void setRetryPause(int ms)                 { m_retryPause = ms; }

    template<Extension ext> inline bool has() const noexcept { return m_extensions.test(ext); }

protected:
    inline void onLine(char *line, size_t size) override { parse(line, size); }

    void onResolved(const Dns &dns, int status) override;

private:
    class Tls;

    bool close();
    bool isCriticalError(const char *message);
    bool parseJob(const rapidjson::Value &params, int *code);
    bool parseLogin(const rapidjson::Value &result, int *code);
    bool send(BIO *bio);
    bool verifyAlgorithm(const Algorithm &algorithm) const;
    int resolve(const String &host);
    int64_t send(const rapidjson::Document &doc);
    int64_t send(size_t size);
    void connect(sockaddr *addr);
    void handshake();
    void login();
    void onClose();
    void parse(char *line, size_t len);
    void parseExtensions(const rapidjson::Value &result);
    void parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &error);
    void parseResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error);
    void ping();
    void read(ssize_t nread);
    void reconnect();
    void setState(SocketState state);
    void startTimeout();

    inline bool isQuiet() const                                   { return m_quiet || m_failures >= m_retries; }
    inline const char *url() const                                { return m_pool.url(); }
    inline void setExtension(Extension ext, bool enable) noexcept { m_extensions.set(ext, enable); }

    static void onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    static void onClose(uv_handle_t *handle);
    static void onConnect(uv_connect_t *req, int status);
    static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

    static inline Client *getClient(void *data) { return m_storage.get(data); }

    bool m_enabled;
    bool m_ipv6;
    bool m_quiet;
    char m_sendBuf[2048];
    const char *m_agent;
    Dns *m_dns;
    IClientListener *m_listener;
    int m_id;
    int m_retries;
    int m_retryPause;
    int64_t m_failures;
    Job m_job;
    Pool m_pool;
    RecvBuf<kInputBufferSize> m_recvBuf;
    SocketState m_state;
    std::bitset<EXT_MAX> m_extensions;
    std::map<int64_t, SubmitResult> m_results;
    String m_ip;
    String m_rpcId;
    Tls *m_tls;
    uint64_t m_expire;
    uint64_t m_jobs;
    uint64_t m_keepAlive;
    uintptr_t m_key;
    uv_stream_t *m_stream;
    uv_tcp_t *m_socket;

    static int64_t m_sequence;
    static Storage<Client> m_storage;
};


template<> inline bool Client::has<Client::EXT_NICEHASH>() const noexcept  { return m_extensions.test(EXT_NICEHASH) || m_pool.isNicehash(); }
template<> inline bool Client::has<Client::EXT_KEEPALIVE>() const noexcept { return m_extensions.test(EXT_KEEPALIVE) || m_pool.keepAlive() > 0; }


} /* namespace xmrig */


#endif /* XMRIG_CLIENT_H */
