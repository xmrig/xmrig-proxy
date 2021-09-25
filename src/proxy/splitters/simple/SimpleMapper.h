/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_SIMPLEMAPPER_H
#define XMRIG_SIMPLEMAPPER_H


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
class NonceStorage;
class Pools;
class SubmitEvent;


class SimpleMapper : public IStrategyListener
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(SimpleMapper)

    SimpleMapper(uint64_t id, Controller *controller);
    ~SimpleMapper() override;

    void add(Miner *miner);
    void reload(const Pools &pools);
    void remove(const Miner *miner);
    void reuse(Miner *miner);
    void stop();
    void submit(SubmitEvent *event);
    void tick(uint64_t ticks, uint64_t now);

    inline bool isActive() const     { return m_active && m_miner; }
    inline bool isReusable() const   { return m_active && !m_miner && !m_dirty; }
    inline uint64_t id() const       { return m_id; }
    inline uint64_t idleTime() const { return m_idleTime; }

protected:
    void onActive(IStrategy *strategy, IClient *client) override;
    void onJob(IStrategy *strategy, IClient *client, const Job &job, const rapidjson::Value &params) override;
    void onLogin(IStrategy *strategy, IClient *client, rapidjson::Document &doc, rapidjson::Value &params) override;
    void onPause(IStrategy *strategy) override;
    void onResultAccepted(IStrategy *strategy, IClient *client, const SubmitResult &result, const char *error) override;
    void onVerifyAlgorithm(IStrategy *strategy, const IClient *client, const Algorithm &algorithm, bool *ok) override;

private:
    bool isValidJobId(const String &id) const;
    void connect();
    void setJob(const Job &job);

    bool m_active               = false;
    bool m_dirty                = false;
    Controller *m_controller;
    DonateStrategy *m_donate    = nullptr;
    IStrategy *m_pending        = nullptr;
    IStrategy *m_strategy;
    Job m_job;
    Job m_prevJob;
    Miner *m_miner              = nullptr;
    uint64_t m_id;
    uint64_t m_idleTime         = 0;
};


} /* namespace xmrig */


#endif /* XMRIG_SIMPLEMAPPER_H */
