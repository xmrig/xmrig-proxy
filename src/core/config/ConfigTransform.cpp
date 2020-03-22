/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "core/config/ConfigTransform.h"
#include "base/kernel/interfaces/IConfig.h"
#include "proxy/BindHost.h"


namespace xmrig
{

static const char *kBind = "bind";

#ifdef XMRIG_FEATURE_TLS
static const char *kTls  = "tls";
#endif

}


void xmrig::ConfigTransform::transform(rapidjson::Document &doc, int key, const char *arg)
{
    BaseTransform::transform(doc, key, arg);

    switch (key) {
    case IConfig::ModeKey: /* --mode */
        return set(doc, "mode", arg);

    case IConfig::BindKey:    /* --bind */
    case IConfig::TlsBindKey: /* --tls-bind */
        {
            BindHost host(arg);
            if (host.isValid()) {
                add(doc, kBind, "host", host.host(), true);
                add(doc, kBind, "port", host.port());

#               ifdef XMRIG_FEATURE_TLS
                add(doc, kBind, kTls,  key == IConfig::TlsBindKey);
#               endif
            }
        }
        break;

    case IConfig::AccessLogFileKey: /* --access-log-file */
        return set(doc, "access-log-file", arg);

    case IConfig::ProxyPasswordKey: /* --access-password */
        return set(doc, "access-password", arg);

    case IConfig::CustomDiffStatsKey: /* --custom-diff-stats */
    case IConfig::DebugKey:   /* --debug */
        return transformBoolean(doc, key, true);

    case IConfig::WorkersKey: /* --no-workers */
    case IConfig::AlgoExtKey: /* --no-algo-ext */
        return transformBoolean(doc, key, false);

    case IConfig::WorkersAdvKey:
        return set(doc, "workers", arg);

    case IConfig::CustomDiffKey: /* --custom-diff */
    case IConfig::ReuseTimeoutKey: /* --reuse-timeout */
        return transformUint64(doc, key, static_cast<uint64_t>(strtol(arg, nullptr, 10)));

    case IConfig::LoginFileKey: /* --login-file */
        return set(doc, "login-file", arg);

    default:
        break;
    }
}


void xmrig::ConfigTransform::transformBoolean(rapidjson::Document &doc, int key, bool enable)
{
    switch (key) {
    case IConfig::CustomDiffStatsKey: /* --custom-diff-stats */
        return set(doc, "custom-diff-stats", enable);

    case IConfig::DebugKey: /* --debug */
        return set(doc, "debug", enable);

    case IConfig::WorkersKey: /* --no-workers */
        return set(doc, "workers", enable);

    case IConfig::AlgoExtKey: /* --no-algo-ext */
        return set(doc, "algo-ext", enable);

    default:
        break;
    }
}


void xmrig::ConfigTransform::transformUint64(rapidjson::Document &doc, int key, uint64_t arg)
{
    switch (key) {
    case IConfig::CustomDiffKey: /* --custom-diff */
        return set(doc, "custom-diff", arg);

    case IConfig::ReuseTimeoutKey: /* --reuse-timeout */
        return set(doc, "reuse-timeout", arg);

    default:
        break;
    }
}
