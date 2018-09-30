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


#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>


#include "common/utils/timestamp.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "log/AccessLog.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"
#include "proxy/Stats.h"


AccessLog::AccessLog(xmrig::Controller *controller) :
    m_file(-1)
{
    const char *fileName = controller->config()->accessLog();
    if (!fileName) {
        return;
    }

    uv_fs_t req;
    m_file = uv_fs_open(uv_default_loop(), &req, fileName, O_CREAT | O_APPEND | O_WRONLY, 0644, nullptr);
    uv_fs_req_cleanup(&req);
}


AccessLog::~AccessLog()
{
}


void AccessLog::onEvent(IEvent *event)
{
    if (m_file < 0) {
        return;
    }

    switch (event->type())
    {
    case IEvent::LoginType: {
            auto e = static_cast<LoginEvent*>(event);
            write("#%" PRId64 " login: %s, \"%s\", ua: \"%s\", count: %" PRIu64, e->miner()->id(), e->miner()->ip(), e->miner()->user(), e->miner()->agent(), Counters::miners());
        }
        break;

    case IEvent::CloseType: {
            auto e = static_cast<CloseEvent*>(event);
            if (e->miner()->mapperId() == -1) {
                break;
            }

            const double time = (double)(xmrig::currentMSecsSinceEpoch() - e->miner()->timestamp()) / 1000;

            write("#%" PRId64 " close: %s, \"%s\", time: %03.1fs, rx/tx: %" PRIu64 "/%" PRIu64 ", count: %" PRIu64,
                  e->miner()->id(), e->miner()->ip(), e->miner()->user(), time, e->miner()->rx(), e->miner()->tx(), Counters::miners());
        }
        break;

    default:
        break;
    }
}


void AccessLog::onRejectedEvent(IEvent *event)
{
}


void AccessLog::onWrite(uv_fs_t *req)
{
    delete [] static_cast<char *>(req->data);

    uv_fs_req_cleanup(req);
    delete req;
}


void AccessLog::write(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    time_t now = time(nullptr);
    tm stime;

#   ifdef _WIN32
    localtime_s(&stime, &now);
#   else
    localtime_r(&now, &stime);
#   endif

    char *buf = new char[1024];
    int size = snprintf(buf, 23, "[%d-%02d-%02d %02d:%02d:%02d] ",
                        stime.tm_year + 1900,
                        stime.tm_mon + 1,
                        stime.tm_mday,
                        stime.tm_hour,
                        stime.tm_min,
                        stime.tm_sec);

    size = vsnprintf(buf + size, 1024 - size - 1, fmt, args) + size;
    buf[size] = '\n';

    uv_buf_t ubuf = uv_buf_init(buf, (unsigned int) size + 1);
    uv_fs_t *req = new uv_fs_t;
    req->data = ubuf.base;

    uv_fs_write(uv_default_loop(), req, m_file, &ubuf, 1, 0, AccessLog::onWrite);

    va_end(args);
}
