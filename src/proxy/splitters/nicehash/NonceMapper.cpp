/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "base/net/Pools.h"
#include "common/log/Log.h"
#include "common/net/Client.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "net/JobResult.h"
#include "net/strategies/DonateStrategy.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/nicehash/NonceMapper.h"
#include "proxy/splitters/nicehash/NonceStorage.h"


xmrig::NonceMapper::NonceMapper(size_t id, Controller *controller) :
    m_controller(controller),
    m_donate(nullptr),
    m_suspended(0),
    m_pending(nullptr),
    m_id(id)
{
    m_storage  = new NonceStorage();
    m_strategy = controller->config()->pools().createStrategy(this);

    if (controller->config()->donateLevel() > 0) {
        m_donate = new DonateStrategy(controller, this);
    }
}


xmrig::NonceMapper::~NonceMapper()
{
    delete m_pending;
    delete m_strategy;
    delete m_storage;
    delete m_donate;
}


bool xmrig::NonceMapper::add(Miner *miner)
{
    if (!m_storage->add(miner)) {
        return false;
    }

    if (isSuspended()) {
        connect();
    }

    miner->setNiceHash(true);
    miner->setMapperId(m_id);
    return true;
}


bool xmrig::NonceMapper::isActive() const
{
    return m_storage->isActive() && !isSuspended();
}


void xmrig::NonceMapper::gc()
{
    if (isSuspended()) {
        m_suspended++;
        return;
    }

    if (m_id == 0 || m_storage->isUsed()) {
        return;
    }

    suspend();
}


void xmrig::NonceMapper::reload(const Pools &pools)
{
    delete m_pending;

    m_pending = pools.createStrategy(this);
    m_pending->connect();
}


void xmrig::NonceMapper::remove(const Miner *miner)
{
    m_storage->remove(miner);
}


void xmrig::NonceMapper::start()
{
    connect();
}


void xmrig::NonceMapper::submit(SubmitEvent *event)
{
    if (!m_storage->isActive()) {
        return event->reject(Error::BadGateway);
    }

    if (!m_storage->isValidJobId(event->request.jobId)) {
        return event->reject(Error::InvalidJobId);
    }

    if (event->request.algorithm.isValid() && event->request.algorithm != m_storage->job().algorithm()) {
        return event->reject(Error::IncorrectAlgorithm);
    }

    JobResult req = event->request;
    req.diff = m_storage->job().diff();

    IStrategy *strategy = m_donate && m_donate->isActive() ? m_donate : m_strategy;

    m_results[strategy->submit(req)] = SubmitCtx(req.id, event->miner()->id());
}


void xmrig::NonceMapper::tick(uint64_t ticks, uint64_t now)
{
    m_strategy->tick(now);

    if (m_donate) {
        m_donate->tick(now);

        if (m_donate->isActive() && m_donate->hasPendingJob() && m_donate->reschedule()) {
            const auto &pending = m_donate->pending();
            setJob(pending.host.data(), pending.port, pending.job);
        }
    }
}


#ifdef APP_DEVEL
void xmrig::NonceMapper::printState()
{
    if (isSuspended()) {
        return;
    }

    m_storage->printState(m_id);
}
#endif


void xmrig::NonceMapper::onActive(IStrategy *strategy, Client *client)
{
    m_storage->setActive(true);

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


void xmrig::NonceMapper::onJob(IStrategy *strategy, Client *client, const Job &job)
{
    if (m_donate) {
        if (m_donate->isActive() && client->id() != -1 && !m_donate->reschedule()) {
            m_donate->save(client, job);
            return;
        }

        m_donate->setAlgo(job.algorithm());
    }

    setJob(client->host(), client->port(), job);
}


void xmrig::NonceMapper::onPause(IStrategy *strategy)
{
    m_storage->setActive(false);

    if (!isSuspended()) {
        LOG_ERR("#%03u no active pools, stop", m_id);
    }
}


void xmrig::NonceMapper::onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error)
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


bool xmrig::NonceMapper::isColors() const
{
    return m_controller->config()->isColors();
}


xmrig::SubmitCtx xmrig::NonceMapper::submitCtx(int64_t seq)
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


void xmrig::NonceMapper::connect()
{
    m_suspended = 0;
    m_strategy->connect();

    if (m_donate) {
        m_donate->connect();
    }
}


void xmrig::NonceMapper::setJob(const char *host, int port, const Job &job)
{
    if (m_controller->config()->isVerbose()) {
        if (job.height()) {
            LOG_INFO(isColors() ? "#%03u " MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d") " diff " WHITE_BOLD("%d") " algo " WHITE_BOLD("%s") " height " WHITE_BOLD("%" PRIu64)
                                : "#%03u new job from %s:%d diff %d algo %s height %" PRIu64,
                     m_id, host, port, job.diff(), job.algorithm().shortName(), job.height());
        }
        else {
            LOG_INFO(isColors() ? "#%03u " MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d") " diff " WHITE_BOLD("%d") " algo " WHITE_BOLD("%s")
                                : "#%03u new job from %s:%d diff %d algo %s",
                     m_id, host, port, job.diff(), job.algorithm().shortName());
        }
    }

    m_storage->setJob(job);
}


void xmrig::NonceMapper::suspend()
{
    m_suspended = 1;
    m_storage->setActive(false);
    m_storage->reset();
    m_strategy->stop();

    if (m_donate) {
        m_donate->stop();
    }
}
