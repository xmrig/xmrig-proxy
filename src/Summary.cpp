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


#include <stdio.h>
#include <uv.h>


#include "core/Config.h"
#include "core/Controller.h"
#include "log/Log.h"
#include "net/Url.h"
#include "proxy/Addr.h"
#include "Summary.h"
#include "version.h"


static void print_versions(xmrig::Controller *controller)
{
    char buf[16];

#   if defined(__clang__)
    snprintf(buf, 16, " clang/%d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#   elif defined(__GNUC__)
    snprintf(buf, 16, " gcc/%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#   elif defined(_MSC_VER)
    snprintf(buf, 16, " MSVC/%d", MSVC_VERSION);
#   else
    buf[0] = '\0';
#   endif


    if (controller->config()->colors()) {
        Log::i()->text("\x1B[01;32m * \x1B[01;37mVERSIONS:     \x1B[01;36mxmrig-proxy/%s\x1B[01;37m libuv/%s%s", APP_VERSION, uv_version_string(), buf);
    } else {
        Log::i()->text(" * VERSIONS:     xmrig-proxy/%s libuv/%s%s", APP_VERSION, uv_version_string(), buf);
    }
}


static void print_mode(xmrig::Controller *controller)
{
    Log::i()->text(controller->config()->colors() ? "\x1B[01;32m * \x1B[01;37mMODE:\x1B[0m         \x1B[01;37m%s" : " * MODE:         %s",
                   controller->config()->modeName());
}


static void print_bind(xmrig::Controller *controller)
{
    const std::vector<Addr*> &addrs = controller->config()->addrs();

    for (size_t i = 0; i < addrs.size(); ++i) {
        Log::i()->text(controller->config()->colors() ? "\x1B[01;32m * \x1B[01;37mBIND #%d:\x1B[0m      \x1B[36m%s%s%s:%d" : " * BIND #%d:      %s%s%s:%d",
                       i + 1,
                       addrs[i]->isIPv6() ? "[" : "",
                       addrs[i]->ip(),
                       addrs[i]->isIPv6() ? "]" : "",
                       addrs[i]->port());
    }
}


#ifndef XMRIG_NO_API
static void print_api(xmrig::Controller *controller)
{
    const int port = controller->config()->apiPort();
    if (port == 0) {
        return;
    }

    Log::i()->text(controller->config()->colors() ? "\x1B[01;32m * \x1B[01;37mAPI BIND:     \x1B[01;36m%s:%d" : " * API BIND:     %s:%d",
                   controller->config()->apiIPv6() ? "[::]" : "0.0.0.0", port);
}
#endif


static void print_commands(xmrig::Controller *controller)
{
    if (controller->config()->colors()) {
        Log::i()->text("\x1B[01;32m * \x1B[01;37mCOMMANDS:     \x1B[01;35mh\x1B[01;37mashrate, \x1B[01;35mc\x1B[01;37monnections, \x1B[01;35mv\x1B[01;37merbose, \x1B[01;35mw\x1B[01;37morkers");
    }
    else {
        Log::i()->text(" * COMMANDS:     'h' hashrate, 'c' connections, 'v' verbose, 'w' workers");
    }
}


void Summary::print(xmrig::Controller *controller)
{
    print_versions(controller);
    print_mode(controller);
    printPools(controller->config());
    print_bind(controller);

#   ifndef XMRIG_NO_API
    print_api(controller);
#   endif

    print_commands(controller);
}


void Summary::printPools(xmrig::Config *config)
{
    const std::vector<Url*> &pools = config->pools();

    for (size_t i = 0; i < pools.size(); ++i) {
        Log::i()->text(config->colors() ? "\x1B[01;32m * \x1B[01;37mPOOL #%d:\x1B[0m      \x1B[36m%s:%d" : " * POOL #%d:      %s:%d",
                       i + 1,
                       pools[i]->host(),
                       pools[i]->port());
    }

#   ifdef APP_DEBUG
    for (size_t i = 0; i < pools.size(); ++i) {
        Log::i()->text("%s:%d, user: %s, pass: %s", pools[i]->host(), pools[i]->port(), pools[i]->user(), pools[i]->password());
    }
#   endif
}



