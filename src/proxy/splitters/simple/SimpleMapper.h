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

#ifndef XMRIG_SIMPLEMAPPER_H
#define XMRIG_SIMPLEMAPPER_H


#include <map>
#include <uv.h>
#include <vector>


#include "common/interfaces/IStrategyListener.h"
#include "common/net/Job.h"
#include "common/net/Pool.h"


class DonateStrategy;
class IStrategy;
class JobResult;
class Miner;
class NonceStorage;
class Options;
class SubmitEvent;
class Url;


namespace xmrig {
    class Controller;
}


class SimpleMapper : public IStrategyListener
{
public:
    SimpleMapper(uint64_t id, xmrig::Controller *controller);
    ~SimpleMapper();

    void add(Miner *miner);
    void reload(const std::vector<Pool> &pools);
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
    void onActive(IStrategy *strategy, Client *client) override;
    void onJob(IStrategy *strategy, Client *client, const Job &job) override;
    void onPause(IStrategy *strategy) override;
    void onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error) override;

private:
    bool isColors() const;
    bool isValidJobId(const xmrig::Id &id) const;
    IStrategy *createStrategy(const std::vector<Pool> &pools);
    void connect();
    void setJob(const Job &job);

    bool m_active;
    bool m_dirty;
    DonateStrategy *m_donate;
    IStrategy *m_pending;
    IStrategy *m_strategy;
    Job m_job;
    Job m_prevJob;
    Miner *m_miner;
    uint64_t m_id;
    uint64_t m_idleTime;
    xmrig::Controller *m_controller;
};


#endif /* XMRIG_SIMPLEMAPPER_H */
