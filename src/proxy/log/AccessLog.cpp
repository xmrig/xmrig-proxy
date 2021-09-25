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

#include "proxy/log/AccessLog.h"
#include "base/io/log/FileLogWriter.h"
#include "base/kernel/events/ConfigEvent.h"
#include "base/kernel/events/SaveEvent.h"
#include "base/kernel/Process.h"
#include "base/tools/Arguments.h"
#include "base/tools/Chrono.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"


#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <ctime>


namespace xmrig {


class AccessLog::Private
{
public:
    constexpr static const char *kKey   = "access-log-file";

    inline bool isOpen() const                          { return writer && writer->isOpen(); }
    inline String read(const ConfigEvent *event) const  { return event->reader()->getString(kKey, fileName); }
    inline void open()                                  { writer = fileName.isEmpty() ? nullptr : std::make_shared<FileLogWriter>(fileName); }
    inline void save(rapidjson::Document &doc) const    { doc.AddMember(rapidjson::StringRef(kKey), fileName.toJSON(), doc.GetAllocator()); }

    void apply(const String &next);
    void close(const CloseEvent *e);
    void login(const LoginEvent *e);
    void write(const char *fmt, ...);

    std::shared_ptr<FileLogWriter> writer;
    String fileName;
};


} // namespace xmrig


xmrig::AccessLog::AccessLog(const ConfigEvent *event) :
    d(std::make_shared<Private>())
{
    d->fileName = Process::arguments().value("--access-log-file");
    d->fileName = d->read(event);
    d->open();
}


void xmrig::AccessLog::onEvent(uint32_t type, IEvent *event)
{
    if (type == IEvent::CONFIG && event->data() == 0 && !event->isRejected()) {
        return d->apply(d->read(static_cast<const ConfigEvent *>(event)));
    }

    if (type == IEvent::SAVE && event->data() == 0) {
        return d->save(static_cast<SaveEvent *>(event)->doc());
    }

    if (!d->isOpen()) {
        return;
    }

    if (type == LOGIN_EVENT && !event->isRejected()) {
        return d->login(static_cast<const LoginEvent *>(event));
    }

    if (type == CLOSE_EVENT && !event->isRejected()) {
        return d->close(static_cast<const CloseEvent *>(event));
    }
}


void xmrig::AccessLog::Private::apply(const String &next)
{
    if (fileName == next) {
        return;
    }

    fileName = next;
    open();
}


void xmrig::AccessLog::Private::close(const CloseEvent *e)
{
    if (e->miner()->mapperId() == -1) {
        return;
    }

    const double time = static_cast<double>(Chrono::currentMSecsSinceEpoch() - e->miner()->timestamp()) / 1000.0;

    write("#%" PRId64 " close: %s, \"%s\", time: %03.1fs, rx/tx: %" PRIu64 "/%" PRIu64 ", count: %" PRIu64,
          e->miner()->id(), e->miner()->ip(), e->miner()->user().data(), time, e->miner()->rx(), e->miner()->tx(), Counters::miners());
}


void xmrig::AccessLog::Private::login(const LoginEvent *e)
{
    write("#%" PRId64 " login: %s, \"%s\", flow: \"%s\", ua: \"%s\", count: %" PRIu64,
          e->miner()->id(), e->miner()->ip(), e->miner()->user().data(), e->flow.data(), e->miner()->agent().data(), Counters::miners());
}


void xmrig::AccessLog::Private::write(const char *fmt, ...)
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

    va_list args{};
    va_start(args, fmt);

    const int rc = vsnprintf(buf + size, sizeof(buf) - size, fmt, args);

    va_end(args);

    if (rc < 0) {
        return;
    }

    writer->writeLine(buf, std::min<size_t>(sizeof(buf), size + rc));
}
