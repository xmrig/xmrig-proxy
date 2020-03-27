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


#include "base/io/log/backends/ConsoleLog.h"
#include "base/io/log/backends/FileLog.h"
#include "base/io/log/Log.h"
#include "base/kernel/interfaces/IControllerListener.h"
#include "common/config/ConfigLoader.h"
#include "common/Platform.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/Proxy.h"


#ifdef HAVE_SYSLOG_H
#   include "base/io/log/backends/SysLog.h"
#endif


#ifdef XMRIG_FEATURE_API
#   include "api/Api.h"
#endif


class xmrig::ControllerPrivate
{
public:
    inline ControllerPrivate(Process *process) :
        process(process),
        proxy(nullptr),
        config(nullptr)
    {}


    inline ~ControllerPrivate()
    {
#       ifdef XMRIG_FEATURE_API
        delete api;
#       endif

        delete proxy;
        delete config;
    }

    Api *api;
    Process *process;
    Proxy *proxy;
    std::vector<IControllerListener *> listeners;
    xmrig::Config *config;
};


xmrig::Controller::Controller(Process *process)
    : d_ptr(new ControllerPrivate(process))
{
}


xmrig::Controller::~Controller()
{
    delete d_ptr;
}


xmrig::Api *xmrig::Controller::api() const
{
    assert(d_ptr->api != nullptr);

    return d_ptr->api;
}


xmrig::Config *xmrig::Controller::config() const
{
    assert(d_ptr->config != nullptr);

    return d_ptr->config;
}


const xmrig::StatsData &xmrig::Controller::statsData() const
{
    return proxy()->statsData();
}


const std::vector<xmrig::Worker> &xmrig::Controller::workers() const
{
    return proxy()->workers();
}


int xmrig::Controller::init()
{
    d_ptr->config = Config::load(d_ptr->process, this);
    if (!d_ptr->config) {
        return 1;
    }

#   ifdef XMRIG_FEATURE_API
    d_ptr->api = new Api(this);
#   endif

    Platform::init(config()->userAgent());

    if (!config()->isBackground()) {
        Log::add(new ConsoleLog());
    }

    if (config()->logFile()) {
        Log::add(new FileLog(config()->logFile()));
    }

#   ifdef HAVE_SYSLOG_H
    if (config()->isSyslog()) {
        Log::add(new SysLog());
    }
#   endif

    d_ptr->proxy = new Proxy(this);
    return 0;
}


xmrig::Proxy *xmrig::Controller::proxy() const
{
    return d_ptr->proxy;
}


std::vector<xmrig::Miner*> xmrig::Controller::miners() const
{
    return proxy()->miners();
}


void xmrig::Controller::addListener(IControllerListener *listener)
{
    d_ptr->listeners.push_back(listener);
}


void xmrig::Controller::save()
{
    if (!config()) {
        return;
    }

    if (d_ptr->config->isShouldSave()) {
        d_ptr->config->save();
    }

    ConfigLoader::watch(d_ptr->config);
}


void xmrig::Controller::start()
{
    proxy()->connect();

#   ifdef XMRIG_FEATURE_API
    api()->start();
#   endif

    save();
}


void xmrig::Controller::stop()
{
#   ifdef XMRIG_FEATURE_API
    api()->stop();
#   endif

    ConfigLoader::release();

    delete d_ptr->proxy;
    d_ptr->proxy = nullptr;
}


void xmrig::Controller::onNewConfig(IConfig *config)
{
    Config *previousConfig = d_ptr->config;
    d_ptr->config = static_cast<Config*>(config);

    for (xmrig::IControllerListener *listener : d_ptr->listeners) {
        listener->onConfigChanged(d_ptr->config, previousConfig);
    }

    delete previousConfig;
}
