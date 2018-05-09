#ifndef __REDISLOG_H__
#define __REDISLOG_H__

#ifndef XMRIG_NO_REDIS

#include <uv.h>
#include <eredis.h>

#include "interfaces/IEventListener.h"


class Stats;

namespace xmrig {
    class Controller;
}

class RedisLog : public IEventListener
{
public:
    RedisLog(xmrig::Controller *controller, const Stats &stats);
    ~RedisLog();

protected:
    void onEvent(IEvent *event) override;
    void onRejectedEvent(IEvent *event) override;

private:
    eredis_t *m_redis;
    const Stats &m_stats;
    xmrig::Controller *m_controller;
};


#endif /* XMRIG_NO_REDIS */
#endif /* __REDISLOG_H__ */
