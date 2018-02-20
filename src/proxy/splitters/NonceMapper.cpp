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
#include <memory>
#include <string.h>


#include "log/Log.h"
#include "net/Client.h"
#include "net/strategies/DonateStrategy.h"
#include "net/strategies/FailoverStrategy.h"
#include "net/strategies/SinglePoolStrategy.h"
#include "net/Url.h"
#include "Options.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/JobResult.h"
#include "proxy/Miner.h"
#include "proxy/splitters/NonceMapper.h"
#include "proxy/splitters/NonceStorage.h"


NonceMapper::NonceMapper(size_t id, const Options *options, const char *agent) :
    m_suspended(false),
    m_agent(agent),
    m_options(options),
    m_donate(nullptr),
    m_id(id)
{
    m_storage = new NonceStorage();
    const std::vector<Url*> &pools = options->pools();

    if (pools.size() > 1) {
        m_strategy = new FailoverStrategy(pools, m_agent, this);
    }
    else {
        m_strategy = new SinglePoolStrategy(pools.front(), m_agent, this);
    }

    if (id != 0 && m_options->donateLevel() > 0) {
        m_donate = new DonateStrategy(m_agent, this);
    }
}


NonceMapper::~NonceMapper()
{
    delete m_strategy;
    delete m_storage;
    delete m_donate;
}


bool NonceMapper::add(Miner *miner, const LoginRequest &request)
{
    if (!m_storage->add(miner, request)) {
        return false;
    }

    if (m_suspended) {
        connect();
    }

    miner->setMapperId(m_id);
    return true;
}


bool NonceMapper::isActive() const
{
    return m_storage->isActive() && !m_suspended;
}


void NonceMapper::gc()
{
    if (m_suspended || m_id == 0 || m_storage->isUsed()) {
        return;
    }

    suspend();
}


void NonceMapper::remove(const Miner *miner)
{
    m_storage->remove(miner);
}


void NonceMapper::start()
{
    connect();
}


void NonceMapper::submit(SubmitEvent *event)
{
    if (!m_storage->isActive()) {
        return event->reject(Error::BadGateway);
    }

    if (!m_storage->isValidJobId(event->request.jobId)) {
        return event->reject(Error::InvalidJobId);
    }

    JobResult req = event->request;
    req.diff = m_storage->job().diff();

    IStrategy *strategy = m_donate && m_donate->isActive() ? m_donate : m_strategy;

    m_results[strategy->submit(req)] = SubmitCtx(req.id, event->miner()->id());
}


void NonceMapper::tick(uint64_t ticks, uint64_t now)
{
    m_strategy->tick(now);

    if (m_donate) {
        m_donate->tick(now);
    }
}


#ifdef APP_DEVEL
void NonceMapper::printState()
{
    if (m_suspended) {
        return;
    }

    m_storage->printState(m_id);
}
#endif


void NonceMapper::onActive(Client *client)
{
    m_storage->setActive(true);

    if (client->id() == -1) {
        return;
    }

    LOG_INFO(m_options->colors() ? "#%03u \x1B[01;37muse pool \x1B[01;36m%s:%d \x1B[01;30m%s" : "#%03u use pool %s:%d %s",
             m_id, client->host(), client->port(), client->ip());
}


void NonceMapper::onJob(Client *client, const Job &job)
{
    if (m_options->verbose()) {
        LOG_INFO(m_options->colors() ? "#%03u \x1B[01;35mnew job\x1B[0m from \x1B[01;37m%s:%d\x1B[0m diff \x1B[01;37m%d" : "#%03u new job from %s:%d diff %d",
                 m_id, client->host(), client->port(), job.diff());
    }

    if (m_donate && m_donate->isActive() && client->id() != -1 && !m_donate->reschedule()) {
        return;
    }

    m_storage->setJob(job);
}


void NonceMapper::onPause(IStrategy *strategy)
{
    m_storage->setActive(false);

    if (!m_suspended) {
        LOG_ERR("#%03u no active pools, stop", m_id);
    }
}


void NonceMapper::onResultAccepted(Client *client, const SubmitResult &result, const char *error)
{
    const SubmitCtx ctx = submitCtx(result.seq);

    AcceptEvent::start(m_id, ctx.miner, result, client->id() == -1, error);

    if (!ctx.miner) {
        return;
    }

    if (error) {
        ctx.miner->replyWithError(ctx.id, error);
    }
    else {
        ctx.miner->success(ctx.id, "OK");
    }
}


SubmitCtx NonceMapper::submitCtx(int64_t seq)
{
    if (!m_results.count(seq)) {
        return SubmitCtx();
    }

    SubmitCtx ctx = m_results.at(seq);
    ctx.miner = m_storage->miner(ctx.minerId);

    auto it = m_results.find(seq);
    if (it != m_results.end()) {
        m_results.erase(it);
    }

    return ctx;
}


void NonceMapper::connect()
{
    m_suspended = false;
    m_strategy->connect();

    if (m_donate) {
        m_donate->connect();
    }
}


void NonceMapper::suspend()
{
    m_suspended = true;
    m_storage->setActive(false);
    m_storage->reset();
    m_strategy->stop();

    if (m_donate) {
        m_donate->stop();
    }
}
