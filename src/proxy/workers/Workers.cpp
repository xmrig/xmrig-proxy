/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
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


#include "api/Api.h"
#include "log/Log.h"
#include "Options.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/LoginRequest.h"
#include "proxy/Miner.h"
#include "proxy/workers/Workers.h"


Workers::Workers() :
    m_enabled(Options::i()->workers())
{
}


Workers::~Workers()
{
}


void Workers::printWorkers()
{
    if (!m_enabled) {
        LOG_ERR("Per worker statistics disabled");

        return;
    }

    char workerName[24];
    size_t size = 0;

    Log::i()->text(Options::i()->colors() ? "\x1B[01;37m%-23s | %-15s | %-5s | %-8s | %-3s | %11s | %11s |" : "%-23s | %-15s | %-5s | %-8s | %-3s | %11s | %11s |",
                   "WORKER NAME", "LAST IP", "COUNT", "ACCEPTED", "REJ", "10 MINUTES", "24 HOURS");

    for (const Worker &worker : m_workers) {
        const char *name = worker.name();
        size = strlen(name);

        if (size > sizeof(workerName) - 1) {
            memcpy(workerName, name, 6);
            memcpy(workerName + 6, "...", 3);
            memcpy(workerName + 9, name + size - sizeof(workerName) + 10, sizeof(workerName) - 10);
            workerName[sizeof(workerName) - 1] = '\0';
        }
        else {
            strncpy(workerName, name, sizeof(workerName) - 1);
        }

        Log::i()->text("%-23s | %-15s | %5" PRIu64 " | %8" PRIu64 " | %3" PRIu64 " | %6.2f kH/s | %6.2f kH/s |",
                       workerName, worker.ip(), worker.connections(), worker.accepted(), worker.rejected(), worker.hashrate(600), worker.hashrate(86400));
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

    Api::tick(m_workers);
}


void Workers::onEvent(IEvent *event)
{
    if (!m_enabled) {
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
    if (!m_enabled) {
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

    const size_t i = m_miners.at(miner->id());
    if (i >= m_workers.size()) {
        return false;
    }

    *index = i;
    return true;
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
    const std::string name(event->request.login());

    if (m_map.count(name) == 0) {
        const size_t id = m_workers.size();
        m_workers.push_back(Worker(id, name, event->miner()->ip()));

        m_map[name] = id;
        m_miners[event->miner()->id()] = id;

        return;
    }

    Worker &worker = m_workers[m_map.at(name)];

    worker.add(event->miner()->ip());
    m_miners[event->miner()->id()] = worker.id();
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
