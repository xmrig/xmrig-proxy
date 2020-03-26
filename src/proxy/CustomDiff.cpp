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


#include <limits.h>
#include <stdlib.h>
#include <string.h>


#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/CustomDiff.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"


xmrig::CustomDiff::CustomDiff(xmrig::Controller *controller) :
    m_controller(controller)
{
}


xmrig::CustomDiff::~CustomDiff()
{
}


void xmrig::CustomDiff::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::LoginType:
        login(static_cast<LoginEvent*>(event));
        break;

    default:
        break;
    }
}


void xmrig::CustomDiff::login(LoginEvent *event)
{
    if (event->miner()->routeId() != -1) {
        return;
    }

    event->miner()->setCustomDiff(m_controller->config()->diff());

    if (event->miner()->user().isNull()) {
        return;
    }

    const char *str = strrchr(event->miner()->user(), '+');
    if (!str) {
        return;
    }

    const unsigned long diff = strtoul(str + 1, nullptr, 10);
    if (diff < 100 || diff >= INT_MAX) {
        return;
    }

    event->miner()->setCustomDiff(diff);
}
