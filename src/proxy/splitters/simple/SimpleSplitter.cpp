/* XMRig
 * Copyright 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "proxy/splitters/simple/SimpleSplitter.h"
#include "base/io/log/Log.h"
#include "proxy/config/MainConfig.h"
#include "core/Controller.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/simple/SimpleMapper.h"
#include "base/kernel/Process.h"
#include "base/tools/Arguments.h"
#include "base/kernel/events/ConfigEvent.h"
#include "base/kernel/events/SaveEvent.h"


#include <cinttypes>


#define LABEL(x) " \x1B[01;30m" x ":\x1B[0m "


namespace xmrig {


static const char *kReuseTimeout    = "reuse-timeout";


} // namespace xmrig


xmrig::SimpleSplitter::SimpleSplitter(xmrig::Controller *controller, const ConfigEvent *event) : Splitter(controller),
    m_reuseTimeout(event->reader()->getUint64(kReuseTimeout, Process::arguments().value("--reuse-timeout").toUint64()))
{
}


xmrig::SimpleSplitter::~SimpleSplitter()
{
    for (SimpleMapper *mapper : m_released) {
        delete mapper;
    }

    for (auto const &kv : m_upstreams) {
        delete kv.second;
    }
}


xmrig::Upstreams xmrig::SimpleSplitter::upstreams() const
{
    uint64_t active = 0;

    for (auto const &kv : m_upstreams) {
        if (kv.second->isActive()) {
           active++;
        }
    }

    return { active, m_idles.size(), m_upstreams.size() };
}


void xmrig::SimpleSplitter::connect()
{
}


void xmrig::SimpleSplitter::gc()
{
}


void xmrig::SimpleSplitter::printConnections()
{
    const Upstreams info = upstreams();

    LOG_INFO("\x1B[01;32m* \x1B[01;37mupstreams\x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("sleep") "\x1B[01;37m%" PRIu64 "\x1B[0m" LABEL("error") "%s%" PRIu64 "\x1B[0m" LABEL("total") "\x1B[01;37m%" PRIu64,
             info.active ? "\x1B[01;32m" : "\x1B[01;31m", info.active, info.sleep, info.error ? "\x1B[01;31m" : "\x1B[01;37m", info.error, m_upstreams.size());

    LOG_INFO("\x1B[01;32m* \x1B[01;37mminers   \x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("max") "\x1B[01;37m%" PRIu64 "\x1B[0m",
             Counters::miners() ? "\x1B[01;32m" : "\x1B[01;31m", Counters::miners(), Counters::maxMiners());
}


void xmrig::SimpleSplitter::tick(uint64_t ticks, uint64_t now)
{
    for (SimpleMapper *mapper : m_released) {
        delete mapper;
    }

    m_released.clear();

    for (auto const &kv : m_upstreams) {
        if (kv.second->idleTime() > m_reuseTimeout) {
            m_released.push_back(kv.second);
            continue;
        }

        kv.second->tick(ticks, now);
    }

    if (m_released.empty()) {
        return;
    }

    for (SimpleMapper *mapper : m_released) {
        stop(mapper);
    }
}


#ifdef APP_DEVEL
void xmrig::SimpleSplitter::printState()
{
}
#endif


void xmrig::SimpleSplitter::onEvent(uint32_t type, IEvent *event)
{
    if (event->isRejected()) {
        return;
    }

    switch (type) {
    case CLOSE_EVENT:
        return remove(static_cast<const CloseEvent *>(event)->miner());

    case LOGIN_EVENT:
        return login(static_cast<const LoginEvent *>(event));

    case SUBMIT_EVENT:
        return submit(static_cast<SubmitEvent *>(event));

    case IEvent::CONFIG:
        m_reuseTimeout = static_cast<const ConfigEvent *>(event)->reader()->getUint64(kReuseTimeout, m_reuseTimeout);
        break;

    case IEvent::SAVE:
        return save(static_cast<SaveEvent *>(event)->doc());

    default:
        break;
    }
}


void xmrig::SimpleSplitter::login(const LoginEvent *event)
{
    if (event->miner()->routeId() != -1) {
        return;
    }

    if (!m_idles.empty()) {
        for (auto const &kv : m_idles) {
            if (kv.second->isReusable()) {
                removeIdle(kv.first);
                kv.second->reuse(event->miner());

                return;
            }
        }
    }

    SimpleMapper *mapper = new SimpleMapper(m_sequence++, m_controller);
    m_upstreams[mapper->id()] = mapper;

    mapper->add(event->miner());
}


void xmrig::SimpleSplitter::stop(SimpleMapper *mapper)
{
    removeIdle(mapper->id());
    removeUpstream(mapper->id());

    mapper->stop();
}


void xmrig::SimpleSplitter::remove(Miner *miner)
{
    const size_t id = static_cast<size_t>(miner->mapperId());
    if (miner->mapperId() < 0 || miner->routeId() != -1 || m_upstreams.count(id) == 0) {
        return;
    }

    SimpleMapper *mapper = m_upstreams[id];
    mapper->remove(miner);

    if (m_reuseTimeout == 0) {
        stop(mapper);

        m_released.push_back(mapper);
    }
    else {
        m_idles[id] = mapper;
    }
}


void xmrig::SimpleSplitter::removeIdle(uint64_t id)
{
    auto it = m_idles.find(id);
    if (it != m_idles.end()) {
        m_idles.erase(it);
    }
}


void xmrig::SimpleSplitter::removeUpstream(uint64_t id)
{
    auto it = m_upstreams.find(id);
    if (it != m_upstreams.end()) {
        m_upstreams.erase(it);
    }
}


void xmrig::SimpleSplitter::save(rapidjson::Document &doc) const
{
    using namespace rapidjson;
    doc.AddMember(StringRef(kReuseTimeout), m_reuseTimeout ? Value(m_reuseTimeout) : Value(kNullType), doc.GetAllocator());
}


void xmrig::SimpleSplitter::submit(SubmitEvent *event)
{
    if (event->miner()->mapperId() < 0 || event->miner()->routeId() != -1) {
        return;
    }

    SimpleMapper *mapper = m_upstreams[event->miner()->mapperId()];
    if (mapper) {
        mapper->submit(event);
    }
}
