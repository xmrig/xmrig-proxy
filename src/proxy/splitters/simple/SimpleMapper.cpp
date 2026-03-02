/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2025 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2025 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "proxy/splitters/simple/SimpleMapper.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/net/stratum/Client.h"
#include "base/net/stratum/Pools.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "net/JobResult.h"
#include "net/strategies/DonateStrategy.h"
#include "proxy/Counters.h"
#include "proxy/Error.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"


#include <cinttypes>


xmrig::SimpleMapper::SimpleMapper(uint64_t id, xmrig::Controller *controller) :
    m_controller(controller),
    m_id(id)
{
    m_strategy = controller->config()->pools().createStrategy(this);
}


xmrig::SimpleMapper::~SimpleMapper()
{
    delete m_pending;
    delete m_strategy;
    delete m_donate;
}


void xmrig::SimpleMapper::add(Miner *miner)
{
    m_miner = miner;

    m_miner->setExtension(Miner::EXT_ALGO,     m_controller->config()->hasAlgoExt());
    m_miner->setExtension(Miner::EXT_NICEHASH, false);
    m_miner->setMapperId(static_cast<ssize_t>(m_id));

    connect();
}


void xmrig::SimpleMapper::reload(const Pools &pools)
{
    delete m_pending;

    m_pending = pools.createStrategy(this);
    m_pending->connect();
}


void xmrig::SimpleMapper::remove(const Miner *)
{
    m_miner = nullptr;
    m_dirty = true;
}


void xmrig::SimpleMapper::reuse(Miner *miner)
{
    m_idleTime = 0;
    m_miner    = miner;
    m_miner->setMapperId(static_cast<ssize_t>(m_id));
}


void xmrig::SimpleMapper::stop()
{
    m_strategy->stop();

    if (m_pending) {
        m_pending->stop();
    }

    if (m_donate) {
        m_donate->stop();
    }
}


void xmrig::SimpleMapper::submit(SubmitEvent *event)
{
    if (!isActive()) {
        return event->setError(Error::BadGateway);
    }

    if (!isValidJobId(event->request.jobId)) {
        return event->setError(Error::InvalidJobId);
    }

    if (event->request.algorithm.isValid() && event->request.algorithm != m_job.algorithm()) {
        return event->setError(Error::IncorrectAlgorithm);
    }

    JobResult req = event->request;
    req.diff = m_job.diff();

    IStrategy *strategy = m_donate && m_donate->isActive() ? m_donate : m_strategy;

    if (strategy) {
        strategy->submit(req);
    }
}


void xmrig::SimpleMapper::tick(uint64_t, uint64_t now)
{
    m_strategy->tick(now);

    if (!m_miner) {
        m_idleTime++;
    }

    if (m_donate) {
        m_donate->tick(now);
    }
}


void xmrig::SimpleMapper::onActive(IStrategy *strategy, IClient *client)
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

        LOG_INFO("%s " CYAN("%04u ") WHITE_BOLD("use %s ") CYAN_BOLD("%s:%d ") GREEN_BOLD("%s") " \x1B[1;30m%s ",
                 Tags::network(), m_id, client->mode(), client->pool().host().data(), client->pool().port(), tlsVersion ? tlsVersion : "", client->ip().data());

        const char *fingerprint = client->tlsFingerprint();
        if (fingerprint != nullptr) {
            LOG_INFO("%s \x1B[1;30mfingerprint (SHA-256): \"%s\"", Tags::network(), fingerprint);
        }
    }
}


void xmrig::SimpleMapper::onJob(IStrategy *, IClient *client, const Job &job, const rapidjson::Value &)
{
    if (m_controller->config()->isVerbose()) {
        LOG_INFO("%s " CYAN("%04u ") MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d") " diff " WHITE_BOLD("%" PRIu64) " algo " WHITE_BOLD("%s") " height " WHITE_BOLD("%" PRIu64),
                 Tags::network(), m_id, client->pool().host().data(), client->pool().port(), job.diff(), job.algorithm().name(), job.height());
    }

    if (m_donate && m_donate->isActive() && client->id() != -1 && !m_donate->reschedule()) {
        return;
    }

    setJob(job);
}


void xmrig::SimpleMapper::onLogin(IStrategy *strategy, IClient *client, rapidjson::Document &doc, rapidjson::Value &params)
{

}


void xmrig::SimpleMapper::onPause(IStrategy *strategy)
{
    if (m_strategy == strategy) {
        m_active = false;
    }
}


void xmrig::SimpleMapper::onResultAccepted(IStrategy *, IClient *client, const SubmitResult &result, const char *error)
{
    AcceptEvent::start(m_id, m_miner, result, client->id() == -1, false, error);

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


void xmrig::SimpleMapper::onVerifyAlgorithm(IStrategy *strategy, const IClient *client, const Algorithm &algorithm, bool *ok)
{

}


bool xmrig::SimpleMapper::isValidJobId(const String &id) const
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


void xmrig::SimpleMapper::connect()
{
    m_strategy->connect();

    if (m_donate) {
        m_donate->connect();
    }
}


void xmrig::SimpleMapper::setJob(const Job &job)
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
