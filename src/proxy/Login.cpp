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

#include "proxy/Login.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/Error.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"


#include <climits>
#include <cstdlib>
#include <cstring>


xmrig::Login::Login(Controller *controller) :
    m_controller(controller)
{
}


xmrig::Login::~Login() = default;


void xmrig::Login::onEvent(IEvent *event)
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


void xmrig::Login::login(LoginEvent *event)
{
    const String &password = m_controller->config()->password();
    if (!password.isNull() && event->miner()->password() != password) {
        return reject(event, Error::toString(Error::Forbidden));
    }
}


void xmrig::Login::reject(LoginEvent *event, const char *message)
{
    event->reject();
    event->miner()->replyWithError(event->loginId, message);
    event->miner()->close();

    if (!m_controller->config()->isVerbose()) {
        return;
    }

    LOG_INFO("%s " RED_BOLD("deny") " " WHITE_BOLD("\"%s\"") " from " CYAN_BOLD("%s") WHITE_BOLD(" (%s)") " reason " RED("\"%s\""),
             Tags::proxy(), event->miner()->rigId(true).data(), event->miner()->ip(), event->miner()->agent().data(), message);
}
