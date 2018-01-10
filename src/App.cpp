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


#include <stdio.h>
#include <stdlib.h>
#include <uv.h>


#include "api/Api.h"
#include "App.h"
#include "Console.h"
#include "log/ConsoleLog.h"
#include "log/FileLog.h"
#include "log/Log.h"
#include "Options.h"
#include "Platform.h"
#include "proxy/Proxy.h"
#include "Summary.h"
#include "version.h"


#ifdef HAVE_SYSLOG_H
#   include "log/SysLog.h"
#endif

#ifndef XMRIG_NO_HTTPD
#   include "api/Httpd.h"
#endif

App *App::m_self = nullptr;



App::App(int argc, char **argv) :
    m_console(nullptr),
    m_httpd(nullptr),
    m_proxy(nullptr),
    m_options(nullptr)
{
    m_self    = this;
    m_options = Options::parse(argc, argv);
    if (!m_options) {
        return;
    }

    Log::init();

    if (!m_options->background()) {
        Log::add(new ConsoleLog(m_options->colors()));
        m_console = new Console(this);
    }

    if (m_options->logFile()) {
        Log::add(new FileLog(m_options->logFile()));
    }

#   ifdef HAVE_SYSLOG_H
    if (m_options->syslog()) {
        Log::add(new SysLog());
    }
#   endif

    Platform::init(m_options->userAgent());

    m_proxy = new Proxy(m_options);

    uv_signal_init(uv_default_loop(), &m_sigHUP);
    uv_signal_init(uv_default_loop(), &m_sigINT);
    uv_signal_init(uv_default_loop(), &m_sigTERM);
}


App::~App()
{
    uv_tty_reset_mode();

    delete m_console;
    delete m_proxy;

#   ifndef XMRIG_NO_HTTPD
    delete m_httpd;
#   endif

    Log::release();
}


int App::exec()
{
    if (!m_options) {
        return 0;
    }

    uv_signal_start(&m_sigHUP,  App::onSignal, SIGHUP);
    uv_signal_start(&m_sigINT,  App::onSignal, SIGINT);
    uv_signal_start(&m_sigTERM, App::onSignal, SIGTERM);

    background();

    Summary::print();

#   ifndef XMRIG_NO_API
    Api::start();
#   endif

#   ifndef XMRIG_NO_HTTPD
    m_httpd = new Httpd(m_options->apiPort(), m_options->apiToken());
    m_httpd->start();
#   endif

    m_proxy->connect();

    const int r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    Options::release();
    Platform::release();

    return r;
}


void App::onConsoleCommand(char command)
{
    switch (command) {
#   ifdef APP_DEVEL
    case 's':
    case 'S':
        m_proxy->printState();
        break;
#   endif

    case 'v':
    case 'V':
        Options::i()->toggleVerbose();
        LOG_NOTICE("verbose: %d", Options::i()->verbose());
        break;

    case 'h':
    case 'H':
        m_proxy->printHashrate();
        break;

    case 'c':
    case 'C':
        m_proxy->printConnections();
        break;

    case 'd':
    case 'D':
        m_proxy->toggleDebug();
        break;

    case 'w':
    case 'W':
        m_proxy->printWorkers();
        break;

    case 3:
        LOG_WARN("Ctrl+C received, exiting");
        close();
        break;

    default:
        break;
    }
}


void App::close()
{
    uv_stop(uv_default_loop());
}


void App::onSignal(uv_signal_t *handle, int signum)
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
        LOG_WARN("signal %d received, ignore", signum);
        return;
    }

    m_self->close();
}
