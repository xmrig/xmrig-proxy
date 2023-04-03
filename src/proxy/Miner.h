/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_MINER_H
#define XMRIG_MINER_H


#include <algorithm>
#include <bitset>
#include <uv.h>


#include "3rdparty/rapidjson/fwd.h"
#include "base/kernel/interfaces/ILineListener.h"
#include "base/net/tools/LineReader.h"
#include "base/net/tools/Storage.h"
#include "base/tools/Object.h"
#include "base/tools/String.h"


using BIO = struct bio_st;


namespace xmrig {


class Job;
class TlsContext;


class Miner : public ILineListener
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(Miner)

    enum State {
        WaitLoginState,
        WaitReadyState,
        ReadyState,
        ClosingState
    };

    enum Extension {
        EXT_ALGO,
        EXT_NICEHASH,
        EXT_CONNECT,
        EXT_MAX
    };

    Miner(const TlsContext *ctx, uint16_t port, bool strictTls);
    ~Miner() override;

    bool accept(uv_stream_t *server);
    void forwardJob(const Job &job, const char *algo);
    void replyWithError(int64_t id, const char *message);
    void setJob(Job &job, int64_t extra_nonce = -1);
    void success(int64_t id, const char *status);

    inline bool hasExtension(Extension ext) const noexcept        { return m_extensions.test(ext); }
    inline const char *ip() const                                 { return m_ip; }
    inline const String &agent() const                            { return m_agent; }
    inline const String &password() const                         { return m_password; }
    inline const String &rigId(bool safe = false) const           { return (safe ? (m_rigId.size() > 0 ? m_rigId : m_user) : m_rigId); }
    inline const String &user() const                             { return m_user; }
    inline int32_t routeId() const                                { return m_routeId; }
    inline int64_t id() const                                     { return m_id; }
    inline ssize_t mapperId() const                               { return m_mapperId; }
    inline State state() const                                    { return m_state; }
    inline uint16_t localPort() const                             { return m_localPort; }
    inline uint64_t customDiff() const                            { return m_customDiff; }
    inline uint64_t diff() const                                  { return (m_customDiff ? std::min(m_customDiff, m_diff) : m_diff); }
    inline uint64_t expire() const                                { return m_expire; }
    inline uint64_t rx() const                                    { return m_rx; }
    inline uint64_t timestamp() const                             { return m_timestamp; }
    inline uint64_t tx() const                                    { return m_tx; }
    inline uint8_t fixedByte() const                              { return m_fixedByte; }
    inline void close()                                           { shutdown(true); }
    inline void setCustomDiff(uint64_t diff)                      { m_customDiff = diff; }
    inline void setExtension(Extension ext, bool enable) noexcept { m_extensions.set(ext, enable); }
    inline void setFixedByte(uint8_t fixedByte)                   { m_fixedByte = fixedByte; }
    inline void setMapperId(ssize_t mapperId)                     { m_mapperId = mapperId; }
    inline void setRouteId(int32_t id)                            { m_routeId = id; }

protected:
    inline void onLine(char *line, size_t size) override          { parse(line, size); }

private:
    class Tls;

    constexpr static size_t kLoginTimeout  = 10 * 1000;
    constexpr static size_t kSocketTimeout = 60 * 10 * 1000;

    bool isWritable() const;
    bool parseRequest(int64_t id, const char *method, const rapidjson::Value &params);
    bool send(BIO *bio);
    void heartbeat();
    void parse(char *line, size_t len);
    void read(ssize_t nread, const uv_buf_t *buf);
    void send(const rapidjson::Document &doc);
    void send(int size);
    void sendJob(const char *blob, const char *jobId, const char *target, const char *algo, uint64_t height, const String &seedHash, const String &signatureKey);
    void setState(State state);
    void shutdown(bool had_error);
    void startTLS(const char *data);

    static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void onTimeout(uv_timer_t *handle);

    inline bool isTLS() const { return m_tls != nullptr; }

    static inline Miner *getMiner(void *data) { return m_storage.get(data); }

    char m_ip[46]{};
    const bool m_strictTls;
    const String m_rpcId;
    const TlsContext *m_tlsCtx;
    int32_t m_routeId       = -1;
    int64_t m_id;
    int64_t m_loginId       = 0;
    LineReader m_reader;
    ssize_t m_mapperId      = -1;
    State m_state           = WaitLoginState;
    std::bitset<EXT_MAX> m_extensions;
    String m_agent;
    String m_password;
    String m_rigId;
    String m_user;
    String m_signatureData;
    uint8_t m_viewTag;
    Tls *m_tls              = nullptr;
    uint16_t m_localPort;
    uint64_t m_customDiff   = 0;
    uint64_t m_diff         = 0;
    uint64_t m_expire;
    uint64_t m_rx           = 0;
    uint64_t m_timestamp;
    uint64_t m_tx           = 0;
    uint8_t m_fixedByte     = 0;
    int64_t m_extraNonce    = -1;
    uintptr_t m_key;
    uv_tcp_t *m_socket;

    static char m_sendBuf[16384];
    static Storage<Miner> m_storage;
};


} /* namespace xmrig */

#endif /* XMRIG_MINER_H */
