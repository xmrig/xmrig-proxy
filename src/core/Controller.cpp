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


#include "core/Controller.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "core/config/Config.h"
#include "proxy/Proxy.h"


#include <cassert>


xmrig::Controller::Controller(Process *process)
    : Base(process),
    m_proxy(nullptr)
{
}


xmrig::Controller::~Controller()
{
    delete m_proxy;
}


int xmrig::Controller::init()
{
    const int rc = Base::init();
    if (rc != 0) {
        return rc;
    }

    m_proxy = new Proxy(this);
    return 0;
}


void xmrig::Controller::start()
{
    Base::start();

    proxy()->connect();
}


void xmrig::Controller::stop()
{
    Base::stop();

    delete m_proxy;
    m_proxy = nullptr;
}


const xmrig::StatsData &xmrig::Controller::statsData() const
{
    return proxy()->statsData();
}


const std::vector<xmrig::Worker> &xmrig::Controller::workers() const
{
    return proxy()->workers();
}


xmrig::Proxy *xmrig::Controller::proxy() const
{
    return m_proxy;
}


std::vector<xmrig::Miner*> xmrig::Controller::miners() const
{
    return proxy()->miners();
}


void xmrig::Controller::execCommand(char command)
{
    switch (command) {
#   ifdef APP_DEVEL
    case 's':
    case 'S':
        proxy()->printState();
        break;
#   endif

    case 'v':
    case 'V':
        config()->toggleVerbose();
        LOG_NOTICE("%s " WHITE_BOLD("verbose: ") CYAN_BOLD("%d"), Tags::config(), config()->isVerbose());
        break;

    case 'h':
    case 'H':
        proxy()->printHashrate();
        break;

    case 'c':
    case 'C':
        proxy()->printConnections();
        break;

    case 'd':
    case 'D':
        proxy()->toggleDebug();
        break;

    case 'w':
    case 'W':
        proxy()->printWorkers();
        break;

    default:
        break;
    }
}
