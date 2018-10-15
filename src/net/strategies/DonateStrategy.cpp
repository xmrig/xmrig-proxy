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


#include "common/crypto/keccak.h"
#include "common/interfaces/IStrategyListener.h"
#include "common/net/Client.h"
#include "common/Platform.h"
#include "common/xmrig.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "donate.h"
#include "net/strategies/DonateStrategy.h"
#include "proxy/Counters.h"
#include "proxy/StatsData.h"


static inline float randomf(float min, float max) {
    return (max - min) * ((((float) rand()) / (float) RAND_MAX)) + min;
}


DonateStrategy::DonateStrategy(xmrig::Controller *controller, IStrategyListener *listener) :
    m_active(false),
    m_listener(listener),
    m_donateTicks(0),
    m_target(0),
    m_ticks(0),
    m_controller(controller)
{
    uint8_t hash[200];
    char userId[65] = { 0 };
    const char *user = controller->config()->pools().front().user();

    xmrig::keccak(reinterpret_cast<const uint8_t *>(user), strlen(user), hash);
    Job::toHex(hash, 32, userId);

    m_client = new Client(-1, Platform::userAgent(), this);

#   ifndef XMRIG_NO_TLS
    m_client->setPool(Pool("donate.ssl.xmrig.com", 8443, userId, nullptr, Pool::kKeepAliveTimeout, false, true));
#   else
    m_client->setPool(Pool("donate.v2.xmrig.com", 5555, userId, nullptr));
#   endif

    m_client->setRetryPause(1000);
    m_client->setAlgo(controller->config()->algorithm());
    m_client->setQuiet(true);

    m_target = (100 - controller->config()->donateLevel()) * 60 * randomf(0.5, 1.5);
}


DonateStrategy::~DonateStrategy()
{
    m_client->deleteLater();
}


bool DonateStrategy::reschedule()
{
    const uint64_t level = m_controller->config()->donateLevel() * 60;
    if (m_donateTicks < level) {
        return false;
    }

    m_target = m_ticks + ((6000 - level) * ((double) m_donateTicks / level));
    m_active = false;

    stop();
    return true;
}


void DonateStrategy::save(const Client *client, const Job &job)
{
    m_pending.job  = job;
    m_pending.host = client->host();
    m_pending.port = client->port();
}


void DonateStrategy::setAlgo(const xmrig::Algorithm &algorithm)
{
    m_client->setAlgo(algorithm);
}


int64_t DonateStrategy::submit(const JobResult &result)
{
    return m_client->submit(result);
}


void DonateStrategy::connect()
{
}


void DonateStrategy::stop()
{
    m_donateTicks = 0;
    m_client->disconnect();
}


void DonateStrategy::tick(uint64_t now)
{
    m_client->tick(now);

    m_ticks++;

    if (m_ticks == m_target) {
        if (kFreeThreshold > 0 && Counters::miners() < kFreeThreshold) {
            m_target += 600;
            return;
        }

        m_pending.job.reset();
        m_client->connect();
    }

    if (isActive()) {
        m_donateTicks++;
    }
}


void DonateStrategy::onClose(Client *client, int failures)
{
    if (!isActive()) {
        return;
    }

    m_active = false;
    m_listener->onPause(this);
}


void DonateStrategy::onJobReceived(Client *client, const Job &job)
{
    if (!isActive()) {
        m_active = true;
        m_listener->onActive(this, client);
    }

    m_listener->onJob(this, client, job);
}


void DonateStrategy::onLoginSuccess(Client *client)
{
}


void DonateStrategy::onResultAccepted(Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}
