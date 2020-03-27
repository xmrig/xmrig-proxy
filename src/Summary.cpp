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


#include <cstdio>
#include <uv.h>


#include "base/io/log/Log.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/BindHost.h"
#include "Summary.h"
#include "version.h"


namespace xmrig {


static void print_mode(xmrig::Controller *controller)
{
    Log::print(GREEN_BOLD(" * ") WHITE_BOLD("%-13s") MAGENTA_BOLD("%s"), "MODE", controller->config()->modeName());
}


static void print_bind(xmrig::Controller *controller)
{
    const BindHosts &bind = controller->config()->bind();

    for (size_t i = 0; i < bind.size(); ++i) {
        Log::print(GREEN_BOLD(" * ") WHITE_BOLD("BIND #%-7zu") CYAN("%s%s%s:") "\x1B[1;%dm%d\x1B[0m",
                          i + 1,
                          bind[i].isIPv6() ? "[" : "",
                          bind[i].host(),
                          bind[i].isIPv6() ? "]" : "",
                          bind[i].isTLS() ? 32 : 36,
                          bind[i].port());
    }
}


static void print_commands(xmrig::Controller *)
{
    if (Log::isColors()) {
        Log::print(GREEN_BOLD(" * ") WHITE_BOLD("COMMANDS     ") MAGENTA_BOLD("h") WHITE_BOLD("ashrate, ")
                                                                        MAGENTA_BOLD("c") WHITE_BOLD("onnections, ")
                                                                        MAGENTA_BOLD("v") WHITE_BOLD("erbose, ")
                                                                        MAGENTA_BOLD("w") WHITE_BOLD("orkers"));
    }
    else {
        Log::print(" * COMMANDS    'h' hashrate, 'c' connections, 'v' verbose, 'w' workers");
    }
}


} // namespace xmrig


void Summary::print(xmrig::Controller *controller)
{
    controller->config()->printVersions();
    print_mode(controller);
    controller->config()->pools().print();
    print_bind(controller);
    print_commands(controller);
}
