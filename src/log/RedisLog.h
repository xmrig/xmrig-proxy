#ifndef __REDISLOG_H__
#define __REDISLOG_H__

#ifndef XMRIG_NO_REDIS

#include <uv.h>
#include <eredis.h>

#include "interfaces/IEventListener.h"


class Stats;
class AcceptEvent;
class LoginEvent;
class CloseEvent;
class Workers;

namespace xmrig {
    class Controller;
}

class RedisLog : public IEventListener
{
public:
    RedisLog(xmrig::Controller *controller, const Stats &stats,
       Workers *workers);
    ~RedisLog();

protected:
    void onEvent(IEvent *event) override;
    void onRejectedEvent(IEvent *event) override;

private:
    void accept(const AcceptEvent *event);
    void reject(const AcceptEvent *event);
    void close(const CloseEvent *event);
    void login(const LoginEvent *event);

    eredis_t *m_redis;
    const Stats &m_stats;
    xmrig::Controller *m_controller;
    Workers *m_workers;
};


#endif /* XMRIG_NO_REDIS */
#endif /* __REDISLOG_H__ */
