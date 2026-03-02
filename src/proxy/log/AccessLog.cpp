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


#include "proxy/log/AccessLog.h"
#include "base/tools/Chrono.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"


#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <ctime>


xmrig::AccessLog::AccessLog(Controller *controller)
{
    const char *fileName = controller->config()->accessLog();
    if (fileName) {
        m_writer.open(fileName);
    }
}


xmrig::AccessLog::~AccessLog() = default;


void xmrig::AccessLog::onEvent(IEvent *event)
{
    if (!m_writer.isOpen()) {
        return;
    }

    switch (event->type())
    {
    case IEvent::LoginType:
        {
            auto e = static_cast<LoginEvent*>(event);
            write("#%" PRId64 " login: %s, \"%s\", flow: \"%s\", ua: \"%s\", count: %" PRIu64,
                  e->miner()->id(), e->miner()->ip(), e->miner()->user().data(), e->flow.data(), e->miner()->agent().data(), Counters::miners());
        }
        break;

    case IEvent::CloseType:
        {
            auto e = static_cast<CloseEvent*>(event);
            if (e->miner()->mapperId() == -1) {
                break;
            }

            const double time = static_cast<double>(Chrono::currentMSecsSinceEpoch() - e->miner()->timestamp()) / 1000.0;

            write("#%" PRId64 " close: %s, \"%s\", time: %03.1fs, rx/tx: %" PRIu64 "/%" PRIu64 ", count: %" PRIu64,
                  e->miner()->id(), e->miner()->ip(), e->miner()->user().data(), time, e->miner()->rx(), e->miner()->tx(), Counters::miners());
        }
        break;

    default:
        break;
    }
}


void xmrig::AccessLog::onRejectedEvent(IEvent *)
{
}


void xmrig::AccessLog::write(const char *fmt, ...)
{
    time_t now = time(nullptr);
    tm stime{};

#   ifdef _WIN32
    localtime_s(&stime, &now);
#   else
    localtime_r(&now, &stime);
#   endif

    static char buf[4096]{};
    int size = snprintf(buf, 23, "[%d-%02d-%02d %02d:%02d:%02d] ",
                        stime.tm_year + 1900,
                        stime.tm_mon + 1,
                        stime.tm_mday,
                        stime.tm_hour,
                        stime.tm_min,
                        stime.tm_sec);

    if (size < 0) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    const int rc = vsnprintf(buf + size, sizeof(buf) - size, fmt, args);

    va_end(args);

    if (rc < 0) {
        return;
    }

    m_writer.writeLine(buf, std::min<size_t>(sizeof(buf), size + rc));
}
