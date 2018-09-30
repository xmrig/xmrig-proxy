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
#include <memory>
#include <string.h>


#include "common/log/Log.h"
#include "common/net/Client.h"
#include "common/net/strategies/FailoverStrategy.h"
#include "common/net/strategies/SinglePoolStrategy.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "net/JobResult.h"
#include "net/strategies/DonateStrategy.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/simple/SimpleMapper.h"


SimpleMapper::SimpleMapper(uint64_t id, xmrig::Controller *controller) :
    m_active(false),
    m_donate(nullptr),
    m_pending(nullptr),
    m_miner(nullptr),
    m_id(id),
    m_idleTime(0),
    m_controller(controller)
{
    m_strategy = createStrategy(controller->config()->pools());

//    if (controller->config()->donateLevel() > 0) {
//        m_donate = new DonateStrategy(id, controller, this);
//    }
}


SimpleMapper::~SimpleMapper()
{
    delete m_pending;
    delete m_strategy;
    delete m_donate;
}


void SimpleMapper::add(Miner *miner)
{
    m_miner = miner;
    m_miner->setMapperId(m_id);

    connect();
}


void SimpleMapper::reload(const std::vector<Pool> &pools)
{
    delete m_pending;

    m_pending = createStrategy(pools);
    m_pending->connect();
}


void SimpleMapper::remove(const Miner *miner)
{
    m_miner = nullptr;
    m_dirty = true;
}


void SimpleMapper::reuse(Miner *miner)
{
    m_idleTime = 0;
    m_miner    = miner;
    m_miner->setMapperId(m_id);
}


void SimpleMapper::stop()
{
    m_strategy->stop();

    if (m_pending) {
        m_pending->stop();
    }

    if (m_donate) {
        m_donate->stop();
    }
}


void SimpleMapper::submit(SubmitEvent *event)
{
    if (!isActive()) {
        return event->reject(Error::BadGateway);
    }

    if (!isValidJobId(event->request.jobId)) {
        return event->reject(Error::InvalidJobId);
    }

    if (event->request.algorithm.isValid() && event->request.algorithm != m_job.algorithm()) {
        return event->reject(Error::IncorrectAlgorithm);
    }

    JobResult req = event->request;
    req.diff = m_job.diff();

    IStrategy *strategy = m_donate && m_donate->isActive() ? m_donate : m_strategy;

    if (strategy) {
        strategy->submit(req);
    }
}


void SimpleMapper::tick(uint64_t ticks, uint64_t now)
{
    m_strategy->tick(now);

    if (!m_miner) {
        m_idleTime++;
    }

    if (m_donate) {
        m_donate->tick(now);
    }
}


void SimpleMapper::onActive(IStrategy *strategy, Client *client)
{
    m_active = true;

    if (client->id() == -1) {
        return;
    }

    if (m_pending && strategy == m_pending) {
        delete m_strategy;

        m_strategy = strategy;
        m_pending  = nullptr;
    }

    if (m_controller->config()->isVerbose()) {
        const char *tlsVersion = client->tlsVersion();

        LOG_INFO(isColors() ? "#%03u " WHITE_BOLD("use pool ") CYAN_BOLD("%s:%d ") GREEN_BOLD("%s") " \x1B[1;30m%s "
                            : "#%03u use pool %s:%d %s %s",
                 m_id, client->host(), client->port(), tlsVersion ? tlsVersion : "", client->ip());

        const char *fingerprint = client->tlsFingerprint();
        if (fingerprint != nullptr) {
            LOG_INFO("\x1B[1;30mfingerprint (SHA-256): \"%s\"", fingerprint);
        }
    }
}


void SimpleMapper::onJob(IStrategy *strategy, Client *client, const Job &job)
{
    if (m_controller->config()->isVerbose()) {
        LOG_INFO(isColors() ? "#%03u " MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d") " diff " WHITE_BOLD("%d") " algo " WHITE_BOLD("%s")
                            : "#%03u new job from %s:%d diff %d algo %s",
                 m_id, client->host(), client->port(), job.diff(), job.algorithm().shortName());
    }

    if (m_donate && m_donate->isActive() && client->id() != -1 && !m_donate->reschedule()) {
        return;
    }

    setJob(job);
}


void SimpleMapper::onPause(IStrategy *strategy)
{
    if (m_strategy == strategy) {
        m_active = false;
    }
}


void SimpleMapper::onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error)
{
    AcceptEvent::start(m_id, m_miner, result, client->id() == -1, error);

    if (!m_miner) {
        return;
    }

    if (error) {
        m_miner->replyWithError(result.reqId, error);
    }
    else {
        m_miner->success(result.reqId, "OK");
    }
}


bool SimpleMapper::isColors() const
{
    return m_controller->config()->isColors();
}


bool SimpleMapper::isValidJobId(const xmrig::Id &id) const
{
    if (m_job.id() == id) {
        return true;
    }

    if (m_prevJob.isValid() && m_prevJob.id() == id) {
        Counters::expired++;
        return true;
    }

    return false;
}


IStrategy *SimpleMapper::createStrategy(const std::vector<Pool> &pools)
{
    const int retryPause = m_controller->config()->retryPause();
    const int retries    = m_controller->config()->retries();

    if (pools.size() > 1) {
        return new FailoverStrategy(pools, retryPause, retries, this);
    }

    return new SinglePoolStrategy(pools.front(), retryPause, retries, this);
}


void SimpleMapper::connect()
{
    m_strategy->connect();

    if (m_donate) {
        m_donate->connect();
    }
}


void SimpleMapper::setJob(const Job &job)
{
    if (m_job.clientId() == job.clientId()) {
        m_prevJob = m_job;
    }
    else {
        m_prevJob.reset();
    }

    m_job   = job;
    m_dirty = false;

    if (m_miner) {
        m_miner->setJob(m_job);
    }
}
