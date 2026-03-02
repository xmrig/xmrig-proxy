/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_EXTRANONCEMAPPER_H
#define XMRIG_EXTRANONCEMAPPER_H


#include <map>
#include <uv.h>
#include <vector>


#include "base/kernel/interfaces/IStrategyListener.h"
#include "base/net/stratum/Job.h"
#include "base/tools/Object.h"


namespace xmrig {


class Controller;
class DonateStrategy;
class IStrategy;
class JobResult;
class Miner;
class ExtraNonceStorage;
class Pools;
class SubmitEvent;


class SubmitCtx
{
public:
    inline SubmitCtx() : id(0), minerId(0), miner(nullptr) {}
    inline SubmitCtx(int64_t id, int64_t minerId) : id(id), minerId(minerId), miner(nullptr) {}

    int64_t id;
    int64_t minerId;
    Miner *miner;
};


class ExtraNonceMapper : public IStrategyListener
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(ExtraNonceMapper)

    ExtraNonceMapper(size_t id, Controller *controller);
    ~ExtraNonceMapper() override;

    bool add(Miner *miner);
    bool isActive() const;
    void gc();
    void reload(const Pools &pools);
    void remove(const Miner *miner);
    void start();
    void submit(SubmitEvent *event);
    void tick(uint64_t ticks, uint64_t now);

    inline bool isSuspended() const { return m_suspended > 0; }
    inline int suspended() const    { return m_suspended; }

#   ifdef APP_DEVEL
    void printState();
#   endif

protected:
    void onActive(IStrategy *strategy, IClient *client) override;
    void onJob(IStrategy *strategy, IClient *client, const Job &job, const rapidjson::Value &params) override;
    void onLogin(IStrategy *strategy, IClient *client, rapidjson::Document &doc, rapidjson::Value &params) override;
    void onPause(IStrategy *strategy) override;
    void onResultAccepted(IStrategy *strategy, IClient *client, const SubmitResult &result, const char *error) override;
    void onVerifyAlgorithm(IStrategy *strategy, const IClient *client, const Algorithm &algorithm, bool *ok) override;

private:
    SubmitCtx submitCtx(int64_t seq);
    void connect();
    void setJob(const char *host, int port, const Job &job);
    void suspend();

    Controller *m_controller;
    DonateStrategy *m_donate    = nullptr;
    int m_suspended             = 0;
    IStrategy *m_pending        = nullptr;
    IStrategy *m_strategy;
    ExtraNonceStorage *m_storage;
    std::map<int64_t, SubmitCtx> m_results;
};


} /* namespace xmrig */


#endif /* XMRIG_EXTRANONCEMAPPER_H */
