/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "proxy/Controller.h"
#include "base/kernel/App.h"
#include "base/kernel/events/ConfigEvent.h"
#include "base/kernel/events/SaveEvent.h"
#include "base/kernel/Process.h"
#include "proxy/config/MainConfig.h"
#include "proxy/Proxy.h"


namespace xmrig {


class Controller::Private
{
public:
    template<typename... Args>
    inline void apply(Args&&... args) {
        apply(std::make_shared<MainConfig>(std::forward<Args>(args)...));
    }

    void apply(std::shared_ptr<MainConfig> next)
    {
        config = std::move(next);
    }

    void start(Controller *controller, const ConfigEvent *event)
    {
        if (!event->isRejected()) {
            apply(*event->reader(), *config);
        }

        if (!proxy) {
            proxy = app->add<Proxy>(controller, event);
        }        
    }

    App *app        = nullptr;
    Proxy *proxy    = nullptr;
    std::shared_ptr<MainConfig> config;
};


} // namespace xmrig


xmrig::Controller::Controller(App *app) :
    d(std::make_shared<Private>())
{
    d->app = app;
    d->apply(Process::arguments());
}


xmrig::Controller::~Controller() = default;


xmrig::App *xmrig::Controller::app() const
{
    return d->app;
}


xmrig::MainConfig *xmrig::Controller::config() const
{
    assert(d->config);

    return d->config.get();
}


void xmrig::Controller::onEvent(uint32_t type, IEvent *event)
{
    if (type == IEvent::CONFIG && event->data() == 0) {
        return d->start(this, static_cast<const ConfigEvent *>(event));
    }

    if (type == IEvent::SAVE && event->data() == 0) {
        return d->config->save(static_cast<SaveEvent *>(event)->doc());
    }
}
