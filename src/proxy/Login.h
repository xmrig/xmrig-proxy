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

#ifndef __LOGIN_H__
#define __LOGIN_H__


#include "interfaces/IEventListener.h"


class LoginEvent;


namespace xmrig {
    class Controller;
}


class Login : public IEventListener
{
public:
    Login(xmrig::Controller *controller);
    ~Login();

protected:
    void onEvent(IEvent *event) override;
    inline void onRejectedEvent(IEvent *event) override {}

private:
    bool verifyAlgorithms(LoginEvent *event);
    void login(LoginEvent *event);
    void reject(LoginEvent *event, const char *message);

    xmrig::Controller *m_controller;
};


#endif /* __LOGIN_H__ */
