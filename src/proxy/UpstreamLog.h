/* XMRig
 * Copyright (c) 2026 MoneroOcean
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 */

#ifndef XMRIG_UPSTREAMLOG_H
#define XMRIG_UPSTREAMLOG_H


#include <cinttypes>
#include <cstddef>


#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/interfaces/IStrategy.h"
#include "base/net/stratum/GetjobCooldown.h"
#include "base/net/stratum/Client.h"
#include "base/net/stratum/Job.h"
#include "base/tools/Chrono.h"

#include <string>


namespace xmrig {


inline std::string upstreamPauseLogKey(const IClient *upstream, size_t group, const char *lastError)
{
    std::string key;
    key.reserve(64);
    const char *host = upstream ? upstream->pool().host().data() : nullptr;
    key += host && host[0] ? host : "unknown";
    key.push_back(':');
    key += std::to_string(upstream ? upstream->pool().port() : 0);
    key += "\ngroup=";
    key += std::to_string(group);
    key += "\n";
    key += lastError ? lastError : "unknown";

    return key;
}


inline bool allowUpstreamPauseLog(const IClient *upstream, size_t group, const char *lastError)
{
    static CapabilityErrorLog log;

    return log.allows(Chrono::steadyMSecs(), upstreamPauseLogKey(upstream, group, lastError));
}


inline void logUpstreamPause(IStrategy *strategy, size_t group, size_t miners, const Job &storageJob)
{
    const IClient *upstream = strategy ? strategy->client() : nullptr;
    const Client *client = dynamic_cast<const Client *>(upstream);
    const char *algo = storageJob.algorithm().isValid() ? storageJob.algorithm().name() : "none";
    const char *lastError = client ? client->lastError() : nullptr;

    if (!lastError || !lastError[0]) {
        lastError = "unknown";
    }

    if (!allowUpstreamPauseLog(upstream, group, lastError)) {
        return;
    }

    if (client) {
        const char *host = upstream->pool().host().data();
        if (!host || !host[0]) {
            host = "unknown";
        }

        LOG_ERR("%s group=%04zu miners=%zu algo=%s upstream=%04" PRIuPTR " endpoint=%s:%u paused: no active upstream; last_error=\"%s\"; reconnecting",
                Tags::network(), group, miners, algo, client->logId(), host, static_cast<unsigned>(upstream->pool().port()), lastError);
    }
    else {
        LOG_ERR("%s group=%04zu miners=%zu algo=%s upstream=unknown endpoint=unknown paused: no active upstream; last_error=\"%s\"; reconnecting",
                Tags::network(), group, miners, algo, lastError);
    }
}


} /* namespace xmrig */


#endif /* XMRIG_UPSTREAMLOG_H */
