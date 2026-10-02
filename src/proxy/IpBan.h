// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef XMRIG_IPBAN_H
#define XMRIG_IPBAN_H


#include <cstdint>
#include <cstddef>
#include <deque>
#include <limits>
#include <string>
#include <unordered_set>


namespace xmrig {


class IpBan
{
public:
    static constexpr uint64_t kDurationMs = 24ULL * 60ULL * 60ULL * 1000ULL;
    static constexpr size_t kCapacity = 65536;

    inline uint64_t durationMs() const { return m_durationMs; }

    inline void setDurationMs(uint64_t durationMs)
    {
        if (m_durationMs == durationMs) {
            return;
        }

        // A single duration keeps FIFO expiry ordered, including after config reloads.
        m_entries.clear();
        m_fifo.clear();
        m_durationMs = durationMs;
    }

    static inline bool isLowDifficulty(const char *error)
    {
        static const char expected[] = "Low difficulty share";
        if (!error) {
            return false;
        }

        for (size_t i = 0;; ++i) {
            if (!expected[i] || !error[i]) {
                return expected[i] == error[i];
            }

            const char left = expected[i] >= 'A' && expected[i] <= 'Z' ? static_cast<char>(expected[i] + ('a' - 'A')) : expected[i];
            const char right = error[i] >= 'A' && error[i] <= 'Z' ? static_cast<char>(error[i] + ('a' - 'A')) : error[i];
            if (left != right) {
                return false;
            }
        }
    }

    inline bool add(const char *ip, uint64_t now)
    {
        if (!m_durationMs || !ip || !*ip) {
            return false;
        }

        prune(now);
        const auto result = m_entries.emplace(ip);
        if (!result.second) {
            return false;
        }

        const uint64_t remaining = std::numeric_limits<uint64_t>::max() - now;
        m_fifo.emplace_back(ip, m_durationMs > remaining ? std::numeric_limits<uint64_t>::max() : now + m_durationMs);
        if (m_entries.size() > kCapacity) {
            m_entries.erase(m_fifo.front().first);
            m_fifo.pop_front();
        }

        return true;
    }

    inline bool isBanned(const char *ip, uint64_t now)
    {
        if (!ip || !*ip) {
            return false;
        }

        prune(now);
        return m_entries.find(ip) != m_entries.end();
    }

    inline void prune(uint64_t now)
    {
        while (!m_fifo.empty()) {
            if (m_fifo.front().second > now) {
                break;
            }

            m_entries.erase(m_fifo.front().first);
            m_fifo.pop_front();
        }
    }

    inline size_t size() const
    {
        return m_entries.size();
    }

private:
    uint64_t m_durationMs = kDurationMs;
    std::unordered_set<std::string> m_entries;
    std::deque<std::pair<std::string, uint64_t>> m_fifo;
};


} /* namespace xmrig */


#endif /* XMRIG_IPBAN_H */
