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


#include <inttypes.h>


#include "common/log/Log.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/workers/Workers.h"
#include "rapidjson/document.h"


#ifdef _MSC_VER
#   define strncasecmp _strnicmp
#   define strcasecmp  _stricmp
#endif


static const char *modes[] = {
    "none",
    "rig_id",
    "user",
    "password",
    "agent",
    "ip"
};


Workers::Workers(xmrig::Controller *controller) :
    m_mode(controller->config()->workersMode()),
    m_controller(controller)
{
    controller->addListener(this);
}


Workers::~Workers()
{
}


void Workers::printWorkers()
{
    if (!isEnabled()) {
        LOG_ERR("Per worker statistics disabled");

        return;
    }

    char workerName[24] = { 0 };
    size_t size = 0;

    Log::i()->text(m_controller->config()->isColors() ? "\x1B[01;37m%-23s | %-15s | %-5s | %-8s | %-3s | %11s | %11s |" : "%-23s | %-15s | %-5s | %-8s | %-3s | %11s | %11s |",
                   "WORKER NAME", "LAST IP", "COUNT", "ACCEPTED", "REJ", "10 MINUTES", "24 HOURS");

    for (const Worker &worker : m_workers) {
        const char *name = worker.name();
        size = strlen(name);

        if (size > sizeof(workerName) - 1) {
            if (m_mode == Agent) {
                memcpy(workerName, name, 20);
                memcpy(workerName + 20, "...", 3);
            }
            else {
                memcpy(workerName, name, 6);
                memcpy(workerName + 6, "...", 3);
                memcpy(workerName + 9, name + size - sizeof(workerName) + 10, sizeof(workerName) - 10);
            }
        }
        else {
            strncpy(workerName, name, sizeof(workerName) - 1);
        }

        Log::i()->text("%-23s | %-15s | %5" PRIu64 " | %8" PRIu64 " | %3" PRIu64 " | %6.2f kH/s | %6.2f kH/s |",
                       workerName, worker.ip(), worker.connections(), worker.accepted(), worker.rejected(), worker.hashrate(600), worker.hashrate(86400));
    }
}


void Workers::reset()
{
    m_miners.clear();
    m_map.clear();
    m_workers.clear();

    if (m_mode == None) {
        return;
    }

    for (const Miner *miner : m_controller->miners()) {
        if (miner->mapperId() != -1) {
            add(miner);
        }
    }
}


void Workers::tick(uint64_t ticks)
{
    if ((ticks % 4) != 0) {
        return;
    }

    for (Worker &worker : m_workers) {
        worker.tick(ticks);
    }
}


const char *Workers::modeName(Mode mode)
{
    return modes[mode];
}


Workers::Mode Workers::parseMode(const char *mode)
{
    constexpr size_t SIZE = sizeof(modes) / sizeof((modes)[0]);

    for (size_t i = 0; i < SIZE; i++) {
        if (strcasecmp(mode, modes[i]) == 0) {
            return static_cast<Mode>(i);
        }
    }

    return RigID;
}


rapidjson::Value Workers::modeToJSON(Mode mode)
{
    using namespace rapidjson;

    switch (mode) {
    case None:
        return Value(kFalseType);

    case RigID:
        return Value(kTrueType);

    default:
        return Value(StringRef(modes[mode]));
    }

}


void Workers::onConfigChanged(xmrig::Config *config, xmrig::Config *previousConfig)
{
    if (m_mode == config->workersMode()) {
        return;
    }

    m_mode = config->workersMode();
    reset();
}


void Workers::onEvent(IEvent *event)
{
    if (!isEnabled()) {
        return;
    }

    switch (event->type())
    {
    case IEvent::LoginType:
        login(static_cast<LoginEvent*>(event));
        break;

    case IEvent::CloseType:
        remove(static_cast<CloseEvent*>(event));
        break;

    case IEvent::AcceptType:
        accept(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


void Workers::onRejectedEvent(IEvent *event)
{
    if (!isEnabled()) {
        return;
    }

    switch (event->type())
    {
    case IEvent::SubmitType:
        reject(static_cast<SubmitEvent*>(event));
        break;

    case IEvent::AcceptType:
        accept(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


bool Workers::indexByMiner(const Miner *miner, size_t *index) const
{
    if (!miner || miner->mapperId() == -1 || m_miners.count(miner->id()) == 0) {
        return false;
    }

    *index = m_miners.at(miner->id());
    return *index < m_workers.size();
}


const char *Workers::nameByMiner(const Miner *miner) const
{
    switch (m_mode) {
    case RigID:
        return miner->rigId(true);

    case User:
        return miner->user();

    case Password:
        return miner->password();

    case Agent:
        return miner->agent();

    case IP:
        return miner->ip();

    default:
        break;
    }

    return nullptr;
}


size_t Workers::add(const Miner *miner)
{
    size_t worker_id = 0;
    const char *name = nameByMiner(miner);
    const std::string key(name == nullptr ? "unknown" : name);

    if (m_map.count(key) == 0) {
        worker_id   = m_workers.size();
        m_map[key] = worker_id;

        m_workers.push_back(Worker(worker_id, key, miner->ip()));
    }
    else {
        worker_id = m_map.at(key);
        m_workers[worker_id].add(miner->ip());
    }

    m_miners[miner->id()] = worker_id;
    return worker_id;
}


void Workers::accept(const AcceptEvent *event)
{
    size_t index = 0;
    if (!indexByMiner(event->miner(), &index)) {
        return;
    }

    Worker &worker = m_workers[index];
    if (!event->isRejected()) {
        worker.add(event->result);
    }
    else {
        worker.reject(false);
    }
}


void Workers::login(const LoginEvent *event)
{
    add(event->miner());
}


void Workers::reject(const SubmitEvent *event)
{
    size_t index = 0;
    if (!indexByMiner(event->miner(), &index)) {
        return;
    }

    m_workers[index].reject(true);
}


void Workers::remove(const CloseEvent *event)
{
    size_t index = 0;
    if (!indexByMiner(event->miner(), &index)) {
        return;
    }

    m_workers[index].remove();
}
