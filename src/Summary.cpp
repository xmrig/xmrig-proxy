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


static void print_mode(xmrig::Controller *controller)
{
    Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("%-13s") MAGENTA_BOLD("%s")
                                                    : " * %-13s%s",
                   "MODE", controller->config()->modeName());
}


static void print_algo(xmrig::Controller *controller)
{
    Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("%-13s") MAGENTA_BOLD("%s")
                                                    : " * %-13s%s",
                   "ALGO", controller->config()->algorithm().name());
}


static void print_bind(xmrig::Controller *controller)
{
    const std::vector<Addr> &addrs = controller->config()->addrs();

    for (size_t i = 0; i < addrs.size(); ++i) {
        Log::i()->text(controller->config()->isColors() ? GREEN_BOLD(" * ") WHITE_BOLD("BIND #%-7zu") CYAN("%s%s%s:") CYAN_BOLD("%d")
                                                        : " * BIND #%-7zu%s%s%s:%d",
                       i + 1,
                       addrs[i].isIPv6() ? "[" : "",
                       addrs[i].ip(),
                       addrs[i].isIPv6() ? "]" : "",
                       addrs[i].port());
    }
}


static void print_commands(xmrig::Controller *controller)
{
    if (controller->config()->isColors()) {
        Log::i()->text(GREEN_BOLD(" * ") WHITE_BOLD("COMMANDS     ") MAGENTA_BOLD("h") WHITE_BOLD("ashrate, ")
                                                                     MAGENTA_BOLD("c") WHITE_BOLD("onnections, ")
                                                                     MAGENTA_BOLD("v") WHITE_BOLD("erbose, ")
                                                                     MAGENTA_BOLD("w") WHITE_BOLD("orkers"));
    }
    else {
        Log::i()->text(" * COMMANDS    'h' hashrate, 'c' connections, 'v' verbose, 'w' workers");
    }
}


void Summary::print(xmrig::Controller *controller)
{
    controller->config()->printVersions();
    print_mode(controller);
    print_algo(controller);
    controller->config()->printPools();
    print_bind(controller);
    controller->config()->printAPI();
    print_commands(controller);
}
