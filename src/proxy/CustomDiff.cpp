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

#include "proxy/CustomDiff.h"
#include "base/kernel/events/ConfigEvent.h"
#include "base/kernel/events/SaveEvent.h"
#include "base/kernel/Process.h"
#include "base/tools/Arguments.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/Miner.h"


#include <cstdlib>
#include <cstring>


namespace xmrig {


class CustomDiff::Private
{
public:
    constexpr static uint32_t kMinDiff  = 100;
    constexpr static uint32_t kMaxDiff  = std::numeric_limits<uint32_t>::max();
    constexpr static const char *kDiff  = "custom-diff";

    inline static uint64_t parseDiff(const char *value)     { return (value && strlen(value) >= 3) ? parseDiff(strtoull(value, nullptr, 0)) : 0; }
    inline static uint64_t parseDiff(const uint64_t value)  { return (value >= kMinDiff && value < kMaxDiff) ? value : 0; }
    inline void read(const ConfigEvent *event)              { setDiff(event->reader()->getUint64(kDiff, m_diff)); }
    inline void setDiff(const char *value)                  { setDiff(parseDiff(value)); }
    inline void setDiff(uint64_t value)                     { m_diff = (value >= kMinDiff && value < kMaxDiff) ? value : 0; }


    inline void save(rapidjson::Document &doc) const
    {
        using namespace rapidjson;
        doc.AddMember(StringRef(kDiff), m_diff ? Value(m_diff) : Value(kNullType), doc.GetAllocator());
    }


    void login(LoginEvent *event) const
    {
        event->miner()->setCustomDiff(m_diff);

        if (event->miner()->user().size() >= 5) {
            const char *str = strrchr(event->miner()->user(), '+');
            uint64_t value  = 0;

            if (str && (value = parseDiff(str + 1))) {
                event->miner()->setCustomDiff(value);
            }
        }
    }

private:
    uint64_t m_diff = 0;
};


} // namespace xmrig


xmrig::CustomDiff::CustomDiff(const ConfigEvent *event) :
    d(std::make_shared<Private>())
{
    d->setDiff(Process::arguments().value("--custom-diff"));

    if (!event->isRejected()) {
        d->read(event);
    }
}


void xmrig::CustomDiff::onEvent(uint32_t type, IEvent *event)
{
    if (type == IEvent::CONFIG && event->data() == 0 && !event->isRejected()) {
        return d->read(static_cast<const ConfigEvent *>(event));
    }

    if (type == IEvent::SAVE && event->data() == 0) {
        return d->save(static_cast<SaveEvent *>(event)->doc());
    }

    if (type == LOGIN_EVENT && event->route() == -1 && !event->isRejected()) {
        return d->login(static_cast<LoginEvent*>(event));
    }
}
