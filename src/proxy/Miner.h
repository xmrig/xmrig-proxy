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

#ifndef __MINER_H__
#define __MINER_H__


#include <algorithm>
#include <uv.h>

#include "proxy/Addr.h"

#include "rapidjson/fwd.h"


class IMinerListener;
class Job;
class RejectEvent;


class Miner
{
public:
    enum State {
        WaitLoginState,
        WaitReadyState,
        ReadyState,
        ClosingState
    };

    Miner(const Addr &addr);
    ~Miner();
    bool accept(uv_stream_t *server);
    void replyWithError(int64_t id, const char *message);
    void setJob(Job &job);
    void success(int64_t id, const char *status);

    inline const char *ip() const                     { return m_ip; }
    inline int64_t id() const                         { return m_id; }
    inline ssize_t mapperId() const                   { return m_mapperId; }
    inline State state() const                        { return m_state; }
    inline uint64_t customDiff() const                { return m_customDiff; }
    inline uint64_t diff() const                      { return (m_customDiff ? std::min(m_customDiff, m_diff) : m_diff); }
    inline uint64_t expire() const                    { return m_expire; }
    inline uint64_t rx() const                        { return m_rx; }
    inline uint64_t timestamp() const                 { return m_timestamp; }
    inline uint64_t tx() const                        { return m_tx; }
    inline uint8_t fixedByte() const                  { return m_fixedByte; }
    inline void close()                               { shutdown(true); }
    inline void setCustomDiff(uint64_t diff)          { m_customDiff = diff; }
    inline void setFixedByte(uint8_t fixedByte)       { m_fixedByte = fixedByte; }
    inline void setMapperId(ssize_t mapperId)         { m_mapperId = mapperId; }

private:
    constexpr static size_t kLoginTimeout  = 10 * 1000;
    constexpr static size_t kSocketTimeout = 60 * 10 * 1000;

    bool parseRequest(int64_t id, const char *method, const rapidjson::Value &params);
    void heartbeat();
    void parse(char *line, size_t len);
    void send(const char *data, int size);
    void send(int size, const bool encrypted = true);
    void setState(State state);
    void shutdown(bool had_error);

    static void onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void onTimeout(uv_timer_t *handle);

    static inline Miner *getMiner(void *data) { return static_cast<Miner*>(data); }

    char m_buf[2048];
    char m_ip[17];
    char m_rpcId[37];
    char m_sendBuf[768];
    char m_keystream[sizeof(m_sendBuf)];
    bool m_encrypted;
    int64_t m_id;
    int64_t m_loginId;
    size_t m_recvBufPos;
    ssize_t m_mapperId;
    State m_state;
    uint64_t m_customDiff;
    uint64_t m_diff;
    uint64_t m_expire;
    uint64_t m_rx;
    uint64_t m_timestamp;
    uint64_t m_tx;
    uint8_t m_fixedByte;
    uv_buf_t m_recvBuf;
    uv_tcp_t m_socket;
};

#endif /* __MINER_H__ */
