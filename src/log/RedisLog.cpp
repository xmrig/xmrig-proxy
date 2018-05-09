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
#include "common/net/SubmitResult.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "log/RedisLog.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/Miner.h"
#include "proxy/Stats.h"
#include "proxy/workers/Workers.h"



RedisLog::RedisLog(xmrig::Controller *controller, const Stats &stats,
                  Workers *workers) :
    m_stats(stats),
    m_controller(controller),
    m_workers(workers)
{

    m_redis = eredis_new();
    // LOG_ERR("connecting to redis: %s:%d", redisHost, redisPort);

    eredis_host_file(m_redis, "redis-hosts.conf" );
    for (int i = 0; i  += sizeof(host_t);)
    eredis_run_thr(m_redis);
}


RedisLog::~RedisLog()
{
    eredis_shutdown(m_redis);
    eredis_free(m_redis);
}


void RedisLog::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::AcceptType:
        accept(static_cast<AcceptEvent*>(event));
        break;

    case IEvent::LoginType:
        login(static_cast<LoginEvent*>(event));
        break;

    case IEvent::CloseType:
        close(static_cast<CloseEvent*>(event));
        break;
    default:
        break;        
    }
}


void RedisLog::onRejectedEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::AcceptType:
        reject(static_cast<AcceptEvent*>(event));
        break;

    default:
        break;
    }
}


void RedisLog::close(const CloseEvent *event)
{
    const Worker *worker;

    if((worker = m_workers->workerByMiner(event->miner())) == nullptr) {
      return;
    }

    eredis_w_cmd(m_redis, "APPEND x:%s @%ld:%s",
         worker->name(), time(nullptr), event->miner()->ip());
}

void RedisLog::login(const LoginEvent *event)
{
    const Worker *worker;

    if((worker = m_workers->workerByMiner(event->miner())) == nullptr) {
      return;
    }

    eredis_w_cmd(m_redis, "APPEND l:%s @%ld:%s",
         worker->name(), time(nullptr), event->miner()->ip());
}

void RedisLog::accept(const AcceptEvent *event)
{
    const Worker *worker;

    if((worker = m_workers->workerByMiner(event->miner())) == nullptr) {
      return;
    }


    eredis_w_cmd(m_redis, "APPEND a:%s @%ld:%u,%.2f",
         worker->name(), time(nullptr), event->result.diff, worker->hashrate(60));

    LOG_INFO("APPEND a:%s @%ld:%u,%.2f",
        worker->name(), time(nullptr), event->result.diff, worker->hashrate(60));
}


void RedisLog::reject(const AcceptEvent *event)
{
    const Worker *worker;

    if((worker = m_workers->workerByMiner(event->miner())) == nullptr) {
      return;
    }


    eredis_w_cmd(m_redis, "APPEND r:%s @%ld:%u,%.2f",
       worker->name(), time(nullptr), event->result.diff, worker->hashrate(600));

    LOG_INFO("APPEND r:%s @%ld:%u,%.2f",
      worker->name(), time(nullptr), event->result.diff, worker->hashrate(600));
}
