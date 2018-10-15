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


#include <limits.h>
#include <stdlib.h>
#include <string.h>


#include "common/log/Log.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Login.h"
#include "proxy/Miner.h"
#include "proxy/Error.h"


Login::Login(xmrig::Controller *controller) :
    m_controller(controller)
{
}


Login::~Login()
{
}


void Login::onEvent(IEvent *event)
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


bool Login::verifyAlgorithms(LoginEvent *event)
{
    if (event->algorithms.empty()) {
        return true;
    }

    const xmrig::Algo baseAlgo = m_controller->config()->algorithm().algo();
    for (const xmrig::Algorithm &algo : event->algorithms) {
        if (algo.algo() == baseAlgo) {
            return true;
        }
    }

    return false;
}


void Login::login(LoginEvent *event)
{
    if (event->algorithms.empty()) {
        return;
    }

    if (!verifyAlgorithms(event)) {
        return reject(event, Error::toString(Error::IncompatibleAlgorithm));
    }
}


void Login::reject(LoginEvent *event, const char *message)
{
    event->reject();
    event->miner()->replyWithError(event->loginId, message);
    event->miner()->close();

    if (!m_controller->config()->isVerbose()) {
        return;
    }

    LOG_INFO(m_controller->config()->isColors() ? RED_BOLD("deny") " " WHITE_BOLD("\"%s\"") " from " CYAN_BOLD("%s") WHITE_BOLD(" (%s)") " reason " RED("\"%s\"")
                                                : "deny \"%s\" from %s (%s) reason \"%s\"",
             event->miner()->rigId(true), event->miner()->ip(), event->miner()->agent(), message);
}
