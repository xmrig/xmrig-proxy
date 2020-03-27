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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>


#ifdef XMRIG_FEATURE_TLS
#   include <openssl/opensslv.h>
#endif


#ifdef XMRIG_AMD_PROJECT
#   if defined(__APPLE__)
#       include <OpenCL/cl.h>
#   else
#       include "3rdparty/CL/cl.h"
#   endif
#endif


#ifdef XMRIG_NVIDIA_PROJECT
#   include "nvidia/cryptonight.h"
#endif


#include "base/io/Json.h"
#include "base/io/log/Log.h"
#include "common/config/CommonConfig.h"
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "version.h"


xmrig::CommonConfig::CommonConfig() :
    m_algorithm(CRYPTONIGHT, VARIANT_AUTO),
    m_adjusted(false),
    m_autoSave(true),
    m_background(false),
    m_dryRun(false),
    m_syslog(false),
    m_upgrade(false),
    m_watch(true),
    m_printTime(60),
    m_state(NoneState)
{
}


void xmrig::CommonConfig::printVersions()
{
    char buf[256] = { 0 };

#   if defined(__clang__)
    snprintf(buf, sizeof buf, "clang/%d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#   elif defined(__GNUC__)
    snprintf(buf, sizeof buf, "gcc/%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#   elif defined(_MSC_VER)
    snprintf(buf, sizeof buf, "MSVC/%d", MSVC_VERSION);
#   endif

    Log::print(GREEN_BOLD(" * ") WHITE_BOLD("%-13s") CYAN_BOLD("%s/%s") WHITE_BOLD(" %s"), "ABOUT", APP_NAME, APP_VERSION, buf);

#   if defined(XMRIG_AMD_PROJECT)
#   if CL_VERSION_2_0
    const char *ocl = "2.0";
#   elif CL_VERSION_1_2
    const char *ocl = "1.2";
#   elif CL_VERSION_1_1
    const char *ocl = "1.1";
#   elif CL_VERSION_1_0
    const char *ocl = "1.0";
#   else
    const char *ocl = "0.0";
#   endif
    int length = snprintf(buf, sizeof buf, "OpenCL/%s ", ocl);
#   elif defined(XMRIG_NVIDIA_PROJECT)
    const int cudaVersion = cuda_get_runtime_version();
    int length = snprintf(buf, sizeof buf, "CUDA/%d.%d ", cudaVersion / 1000, cudaVersion % 100);
#   else
    memset(buf, 0, 16);

#   if defined(XMRIG_FEATURE_HTTP) || defined(XMRIG_FEATURE_TLS)
    int length = 0;
#   endif
#   endif

#   if defined(XMRIG_FEATURE_TLS) && defined(OPENSSL_VERSION_TEXT)
    {
        constexpr const char *v = OPENSSL_VERSION_TEXT + 8;
        length += snprintf(buf + length, (sizeof buf) - length, "OpenSSL/%.*s ", static_cast<int>(strchr(v, ' ') - v), v);
    }
#   endif

    Log::print(GREEN_BOLD(" * ") WHITE_BOLD("%-13slibuv/%s %s"), "LIBS", uv_version_string(), buf);
}


bool xmrig::CommonConfig::save()
{
    if (m_fileName.isNull()) {
        return false;
    }

    rapidjson::Document doc;
    getJSON(doc);

    if (Json::save(m_fileName, doc)) {
        LOG_NOTICE("configuration saved to: \"%s\"", m_fileName.data());
        return true;
    }

    return false;
}


bool xmrig::CommonConfig::finalize()
{
    if (m_state == ReadyState) {
        return true;
    }

    if (m_state == ErrorState) {
        return false;
    }

    if (!m_algorithm.isValid()) {
        return false;
    }

    m_pools.adjust(m_algorithm);

    if (!m_pools.active()) {
        m_state = ErrorState;
        return false;
    }

    m_state = ReadyState;
    return true;
}


bool xmrig::CommonConfig::parseBoolean(int key, bool enable)
{
    switch (key) {
    case BackgroundKey: /* --background */
        m_background = enable;
        break;

    case SyslogKey: /* --syslog */
        m_syslog = enable;
        break;

    case KeepAliveKey: /* --keepalive */
        m_pools.setKeepAlive(enable);
        break;

    case TlsKey: /* --tls */
        m_pools.setTLS(enable);
        break;

#   ifndef XMRIG_PROXY_PROJECT
    case NicehashKey: /* --nicehash */
        m_pools.setNicehash(enable);
        break;
#   endif

    case ColorKey: /* --no-color */
        Log::colors = enable;
        break;

    case WatchKey: /* watch */
        m_watch = enable;
        break;

#   ifdef XMRIG_DEPRECATED
    case ApiIPv6Key: /* --api-ipv6 */
        break;

    case ApiRestrictedKey: /* --api-no-restricted */
        fputs("option \"--api-no-restricted\" deprecated, use \"--http-no-restricted\" instead.\n", stderr);
        fflush(stdout);
        m_http.setRestricted(enable);
        break;
#   endif

    case HttpRestrictedKey: /* --http-no-restricted */
        m_http.setRestricted(enable);
        break;

    case HttpEnabledKey: /* --http-enabled */
        m_http.setEnabled(enable);
        break;

    case DryRunKey: /* --dry-run */
        m_dryRun = enable;
        break;

    case AutoSaveKey:
        m_autoSave = enable;
        break;

    default:
        break;
    }

    return true;
}


bool xmrig::CommonConfig::parseString(int key, const char *arg)
{
    switch (key) {
    case AlgorithmKey: /* --algo */
        m_algorithm.parseAlgorithm(arg);
        break;

    case UserpassKey: /* --userpass */
        return m_pools.setUserpass(arg);

    case UrlKey: /* --url */
        return m_pools.setUrl(arg);

    case UserKey: /* --user */
        m_pools.setUser(arg);
        break;

    case PasswordKey: /* --pass */
        m_pools.setPassword(arg);
        break;

    case RigIdKey: /* --rig-id */
        m_pools.setRigId(arg);
        break;

    case FingerprintKey: /* --tls-fingerprint */
        m_pools.setFingerprint(arg);
        break;

    case VariantKey: /* --variant */
        m_pools.setVariant(arg);
        break;

    case LogFileKey: /* --log-file */
        m_logFile = arg;
        break;

#   ifdef XMRIG_DEPRECATED
    case ApiAccessTokenKey: /* --api-access-token */
        fputs("option \"--api-access-token\" deprecated, use \"--http-access-token\" instead.\n", stderr);
        fflush(stdout);
        m_http.setToken(arg);
        break;
#   endif

    case HttpAccessTokenKey: /* --http-access-token */
        m_http.setToken(arg);
        break;

    case HttpHostKey: /* --http-host */
        m_http.setHost(arg);
        break;

    case ApiWorkerIdKey: /* --api-worker-id */
        m_apiWorkerId = arg;
        break;

    case ApiIdKey: /* --api-id */
        m_apiId = arg;
        break;

    case UserAgentKey: /* --user-agent */
        m_userAgent = arg;
        break;

    case RetriesKey:     /* --retries */
    case RetryPauseKey:  /* --retry-pause */
    case PrintTimeKey:   /* --print-time */
    case HttpPort:       /* --http-port */
#   ifdef XMRIG_DEPRECATED
    case ApiPort:       /* --api-port */
#   endif
        return parseUint64(key, static_cast<uint64_t>(strtol(arg, nullptr, 10)));

    case BackgroundKey:  /* --background */
    case SyslogKey:      /* --syslog */
    case KeepAliveKey:   /* --keepalive */
    case NicehashKey:    /* --nicehash */
    case TlsKey:         /* --tls */
    case DryRunKey:      /* --dry-run */
    case HttpEnabledKey: /* --http-enabled */
        return parseBoolean(key, true);

    case ColorKey:          /* --no-color */
    case WatchKey:          /* --no-watch */
    case HttpRestrictedKey: /* --http-no-restricted */
#   ifdef XMRIG_DEPRECATED
    case ApiRestrictedKey: /* --api-no-restricted */
    case ApiIPv6Key:       /* --api-ipv6 */
#   endif
        return parseBoolean(key, false);

    case DonateLevelKey: /* --donate-level */
        return parseUint64(key, static_cast<uint64_t>(strtol(arg, nullptr, 10)));

    default:
        break;
    }

    return true;
}


bool xmrig::CommonConfig::parseUint64(int key, uint64_t arg)
{
    return parseInt(key, static_cast<int>(arg));
}


void xmrig::CommonConfig::parseJSON(const rapidjson::Value &json)
{
    const rapidjson::Value &pools = json["pools"];
    if (pools.IsArray()) {
        m_pools.load(pools);
    }

#   ifdef XMRIG_DEPRECATED
    const rapidjson::Value &api = json["api"];
    if (api.IsObject() && api.HasMember("port")) {
        m_upgrade = true;
        m_http.load(api);
    }
    else {
        m_http.load(json["http"]);
    }
#   else
    m_http.load(doc["http"]);
#   endif
}


void xmrig::CommonConfig::setFileName(const char *fileName)
{
    m_fileName = fileName;
}


bool xmrig::CommonConfig::parseInt(int key, int arg)
{
    switch (key) {
    case RetriesKey: /* --retries */
        m_pools.setRetries(arg);
        break;

    case RetryPauseKey: /* --retry-pause */
        m_pools.setRetryPause(arg);
        break;

    case KeepAliveKey: /* --keepalive */
        m_pools.setKeepAlive(arg);
        break;

    case VariantKey: /* --variant */
        m_pools.setVariant(arg);
        break;

    case DonateLevelKey: /* --donate-level */
        m_pools.setDonateLevel(arg);
        break;

    case ProxyDonateKey: /* --donate-over-proxy */
        m_pools.setProxyDonate(arg);
        break;

#   ifdef XMRIG_DEPRECATED
    case ApiPort: /* --api-port */
        fputs("option \"--api-port\" deprecated, use \"--http-port\" instead.\n", stderr);
        fflush(stdout);
        m_http.setPort(arg);
        break;
#   endif

    case HttpPort: /* --http-port */
        m_http.setPort(arg);
        break;

    case PrintTimeKey: /* --print-time */
        if (arg >= 0 && arg <= 3600) {
            m_printTime = arg;
        }
        break;

    default:
        break;
    }

    return true;
}
