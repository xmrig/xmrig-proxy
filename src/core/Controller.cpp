/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2018 XMRig       <support@xmrig.com>
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


#include "common/config/ConfigLoader.h"
#include "common/log/ConsoleLog.h"
#include "common/log/FileLog.h"
#include "common/log/Log.h"
#include "common/Platform.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "interfaces/IControllerListener.h"
#include "proxy/Proxy.h"


#ifdef HAVE_SYSLOG_H
#   include "common/log/SysLog.h"
#endif


class xmrig::ControllerPrivate
{
public:
    inline ControllerPrivate() :
        proxy(nullptr),
        config(nullptr)
    {}


    inline ~ControllerPrivate()
    {
        delete proxy;
        delete config;
    }


    Proxy *proxy;
    xmrig::Config *config;
    std::vector<xmrig::IControllerListener *> listeners;
};


xmrig::Controller::Controller()
    : d_ptr(new ControllerPrivate())
{

}


xmrig::Controller::~Controller()
{
    ConfigLoader::release();

    delete d_ptr;
}


xmrig::Config *xmrig::Controller::config() const
{
    return d_ptr->config;
}


const StatsData &xmrig::Controller::statsData() const
{
    return proxy()->statsData();
}


const std::vector<Worker> &xmrig::Controller::workers() const
{
    return proxy()->workers();
}


int xmrig::Controller::init(int argc, char **argv)
{
    d_ptr->config = xmrig::Config::load(argc, argv, this);
    if (!d_ptr->config) {
        return 1;
    }

    Log::init();
    Platform::init(config()->userAgent());

    if (!config()->isBackground()) {
        Log::add(new ConsoleLog(this));
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


Proxy *xmrig::Controller::proxy() const
{
    return d_ptr->proxy;
}


void xmrig::Controller::addListener(IControllerListener *listener)
{
    d_ptr->listeners.push_back(listener);
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
