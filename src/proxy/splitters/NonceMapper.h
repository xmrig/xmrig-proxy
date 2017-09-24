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

#ifndef __NONCEMAPPER_H__
#define __NONCEMAPPER_H__


#include <map>
#include <uv.h>
#include <vector>


#include "interfaces/IStrategyListener.h"
#include "net/Job.h"


class DonateStrategy;
class IStrategy;
class JobResult;
class LoginRequest;
class Miner;
class NonceStorage;
class Options;
class SubmitEvent;
class Url;


class SubmitCtx
{
public:
    inline SubmitCtx() : id(0), minerId(0), miner(nullptr) {}
    inline SubmitCtx(int64_t id, int64_t minerId) : id(id), minerId(minerId), miner(nullptr) {}

    int64_t id;
    int64_t minerId;
    Miner *miner;
};


class NonceMapper : public IStrategyListener
{
public:
    NonceMapper(size_t id, const Options *options, const char *agent);
    ~NonceMapper();

    bool add(Miner *miner, const LoginRequest &request);
    bool isActive() const;
    void gc();
    void remove(const Miner *miner);
    void start();
    void submit(SubmitEvent *event);
    void tick(uint64_t ticks, uint64_t now);

    inline bool isSuspended() const { return m_suspended; }

#   ifdef APP_DEVEL
    void printState();
#   endif

protected:
    void onActive(Client *client) override;
    void onJob(Client *client, const Job &job) override;
    void onPause(IStrategy *strategy) override;
    void onResultAccepted(Client *client, const SubmitResult &result, const char *error) override;

private:
    SubmitCtx submitCtx(int64_t seq);
    void connect();
    void suspend();

    bool m_suspended;
    const char *m_agent;
    const Options *m_options;
    DonateStrategy *m_donate;
    IStrategy *m_strategy;
    NonceStorage *m_storage;
    size_t m_id;
    std::map<int64_t, SubmitCtx> m_results;
};


#endif /* __NONCEMAPPER_H__ */
