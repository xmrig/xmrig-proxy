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

#include <stdio.h>
#include <stdlib.h>
#include <uv.h>


#include "api/Api.h"
#include "App.h"
#include "base/kernel/Signals.h"
#include "common/Console.h"
#include "common/log/Log.h"
#include "common/Platform.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "proxy/Proxy.h"
#include "Summary.h"
#include "version.h"

#ifndef XMRIG_NO_HTTPD
#   include "common/api/Httpd.h"
#endif


xmrig::App::App(Process *process) :
    m_console(nullptr),
    m_httpd(nullptr),
    m_signals(nullptr)
{
    m_controller = new Controller(process);
    if (m_controller->init() != 0) {
        return;
    }

    if (!m_controller->config()->isBackground()) {
        m_console = new Console(this);
    }
}


xmrig::App::~App()
{
    uv_tty_reset_mode();

    delete m_signals;
    delete m_console;
    delete m_controller;

#   ifndef XMRIG_NO_HTTPD
    delete m_httpd;
#   endif

    Log::release();
}


int xmrig::App::exec()
{
    if (!m_controller->config()) {
        return 2;
    }

    m_signals = new Signals(this);

    background();

    Summary::print(m_controller);

#   ifndef XMRIG_NO_API
    Api::start(m_controller);
#   endif

#   ifndef XMRIG_NO_HTTPD
    m_httpd = new Httpd(
                m_controller->config()->apiPort(),
                m_controller->config()->apiToken(),
                m_controller->config()->isApiIPv6(),
                m_controller->config()->isApiRestricted()
                );

    m_httpd->start();
#   endif

    m_controller->watch();
    m_controller->proxy()->connect();

    const int r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    return r;
}


void xmrig::App::onConsoleCommand(char command)
{
    switch (command) {
#   ifdef APP_DEVEL
    case 's':
    case 'S':
        m_controller->proxy()->printState();
        break;
#   endif

    case 'v':
    case 'V':
        m_controller->config()->toggleVerbose();
        LOG_NOTICE("verbose: %d", m_controller->config()->isVerbose());
        break;

    case 'h':
    case 'H':
        m_controller->proxy()->printHashrate();
        break;

    case 'c':
    case 'C':
        m_controller->proxy()->printConnections();
        break;

    case 'd':
    case 'D':
        m_controller->proxy()->toggleDebug();
        break;

    case 'w':
    case 'W':
        m_controller->proxy()->printWorkers();
        break;

    case 3:
        LOG_WARN("Ctrl+C received, exiting");
        close();
        break;

    default:
        break;
    }
}


void xmrig::App::onSignal(int signum)
{
    switch (signum)
    {
    case SIGHUP:
        LOG_WARN("SIGHUP received, exiting");
        break;

    case SIGTERM:
        LOG_WARN("SIGTERM received, exiting");
        break;

    case SIGINT:
        LOG_WARN("SIGINT received, exiting");
        break;

    default:
        return;
    }

    close();
}


void xmrig::App::close()
{
    uv_stop(uv_default_loop());
}
