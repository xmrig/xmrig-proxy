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


#include "common/log/Log.h"
#include "common/net/Pool.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "proxy/Addr.h"
#include "Summary.h"
#include "version.h"


static void print_versions(xmrig::Controller *controller)
{
    char buf[16] = { 0 };

#   if defined(__clang__)
    snprintf(buf, 16, " clang/%d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#   elif defined(__GNUC__)
    snprintf(buf, 16, " gcc/%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#   elif defined(_MSC_VER)
    snprintf(buf, 16, " MSVC/%d", MSVC_VERSION);
#   else
    buf[0] = '\0';
#   endif


    Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("%-12s") CYAN_BOLD("%s/%s") WHITE_BOLD(" libuv/%s%s")
                                                    : " * %-12s%s/%s libuv/%s%s",
                   "VERSIONS", APP_NAME, APP_VERSION, uv_version_string(), buf);
}


static void print_mode(xmrig::Controller *controller)
{
    Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("%-12s") MAGENTA_BOLD("%s")
                                                    : " * %-12s%s",
                   "MODE", controller->config()->modeName());
}


static void print_algo(xmrig::Controller *controller)
{
    Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("%-12s") MAGENTA_BOLD("%s")
                                                    : " * %-12s%s",
                   "ALGO", controller->config()->algorithm().name());
}


static void print_bind(xmrig::Controller *controller)
{
    const std::vector<Addr> &addrs = controller->config()->addrs();

    for (size_t i = 0; i < addrs.size(); ++i) {
        Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("BIND #%-6d") CYAN("%s%s%s:") CYAN_BOLD("%d")
                                                        : " * BIND #%-6d%s%s%s:%d",
                       i + 1,
                       addrs[i].isIPv6() ? "[" : "",
                       addrs[i].ip(),
                       addrs[i].isIPv6() ? "]" : "",
                       addrs[i].port());
    }
}


#ifndef XMRIG_NO_API
static void print_api(xmrig::Controller *controller)
{
    const int port = controller->config()->apiPort();
    if (port == 0) {
        return;
    }

    Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("%-12s") CYAN("%s:") CYAN_BOLD("%d")
                                                    : " * %-12s%s:%d",
                   "API BIND", controller->config()->isApiIPv6() ? "[::]" : "0.0.0.0", port);
}
#endif


static void print_commands(xmrig::Controller *controller)
{
    if (controller->config()->isColors()) {
        Log::i()->text(GREEN_BOLD(" * ") WHITE_BOLD("COMMANDS    ") MAGENTA_BOLD("h") WHITE_BOLD("ashrate, ")
                                                                    MAGENTA_BOLD("c") WHITE_BOLD("onnections, ")
                                                                    MAGENTA_BOLD("v") WHITE_BOLD("erbose, ")
                                                                    MAGENTA_BOLD("w") WHITE_BOLD("orkers"));
    }
    else {
        Log::i()->text(" * COMMANDS:   'h' hashrate, 'c' connections, 'v' verbose, 'w' workers");
    }
}


void Summary::print(xmrig::Controller *controller)
{
    print_versions(controller);
    print_mode(controller);
    print_algo(controller);
    printPools(controller->config());
    print_bind(controller);

#   ifndef XMRIG_NO_API
    print_api(controller);
#   endif

    print_commands(controller);
}


void Summary::printPools(xmrig::Config *config)
{
    const std::vector<Pool> &pools = config->pools();

    for (size_t i = 0; i < pools.size(); ++i) {
        Log::i()->text(config->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("POOL #%-6zu") CYAN_BOLD("%s") " variant " WHITE_BOLD("%s")
                                          : " * POOL #%-6d%s variant %s",
                       i + 1,
                       pools[i].url(),
                       pools[i].algorithm().variantName()
                       );
    }

#   ifdef APP_DEBUG
    for (const Pool &pool : pools) {
        pool.print();
    }
#   endif
}



