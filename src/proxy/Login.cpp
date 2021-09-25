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

#include "proxy/Login.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/events/ConfigEvent.h"
#include "base/kernel/events/SaveEvent.h"
#include "base/kernel/Process.h"
#include "base/tools/Arguments.h"
#include "proxy/Error.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"


#include <cstdlib>
#include <cstring>


namespace xmrig {


class Login::Private
{
public:
    constexpr static const char *kKey   = "access-password";

    inline void read(const ConfigEvent *event)          { password = event->reader()->getString(kKey, password); }
    inline void save(rapidjson::Document &doc) const    { doc.AddMember(rapidjson::StringRef(kKey), password.toJSON(), doc.GetAllocator()); }

    void login(LoginEvent *event) const;

    String password;
};


} // namespace xmrig


xmrig::Login::Login(const ConfigEvent *event) :
    d(std::make_shared<Private>())
{
    d->password = Process::arguments().value("--access-password");
    d->read(event);
}


void xmrig::Login::onEvent(uint32_t type, IEvent *event)
{
    if (type == IEvent::CONFIG && event->data() == 0 && !event->isRejected()) {
        return d->read(static_cast<const ConfigEvent *>(event));
    }

    if (type == IEvent::SAVE && event->data() == 0) {
        return d->save(static_cast<SaveEvent *>(event)->doc());
    }

    if (type == LOGIN_EVENT && !event->isRejected()) {
        return d->login(static_cast<LoginEvent *>(event));;
    }
}


void xmrig::Login::Private::login(LoginEvent *event) const
{
    if (password.isNull() || event->miner()->password() == password) {
        return;
    }

    const char *message = Error::toString(Error::Forbidden);

    event->reject();
    event->miner()->replyWithError(event->loginId, message);
    event->miner()->close();

    LOG_V1("%s " RED_BOLD("deny") " " WHITE_BOLD("\"%s\"") " from " CYAN_BOLD("%s") WHITE_BOLD(" (%s)") " reason " RED("\"%s\""),
           Tags::proxy(), event->miner()->rigId(true).data(), event->miner()->ip(), event->miner()->agent().data(), message);
}
