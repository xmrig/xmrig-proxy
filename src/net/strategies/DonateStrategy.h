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

#ifndef XMRIG_DONATESTRATEGY_H
#define XMRIG_DONATESTRATEGY_H


#include "base/kernel/interfaces/IClientListener.h"
#include "base/kernel/interfaces/IStrategy.h"
#include "base/net/stratum/Job.h"
#include "base/tools/String.h"


namespace xmrig {


class Client;
class IStrategyListener;
class Controller;


class DonateStrategy : public IStrategy, public IClientListener
{
public:
    struct Pending
    {
        Job job;
        String host;
        int port;
    };


    DonateStrategy(Controller *controller, IStrategyListener *listener);
    ~DonateStrategy() override;

    bool reschedule();
    void save(const IClient *client, const Job &job);
    void setAlgo(const xmrig::Algorithm &algorithm) override;

    inline bool hasPendingJob() const     { return m_pending.job.isValid(); }
    inline const Pending &pending() const { return m_pending; }

    inline bool isActive() const override  { return m_active; }
    inline IClient *client() const override { return m_client; }
    inline void resume() override          {}

    int64_t submit(const JobResult &result) override;
    void connect() override;
    void stop() override;
    void tick(uint64_t now) override;

protected:
    void onClose(IClient *client, int failures) override;
    void onJobReceived(IClient *client, const Job &job, const rapidjson::Value &params) override;
    void onLogin(IClient *client, rapidjson::Document &doc, rapidjson::Value &params) override;
    void onLoginSuccess(IClient *client) override;
    void onResultAccepted(IClient *client, const SubmitResult &result, const char *error) override;
    void onVerifyAlgorithm(const IClient *client, const Algorithm &algorithm, bool *ok) override;
    void setProxy(const ProxyUrl &proxy) override;

private:
    bool m_active;
    IClient *m_client;
    Controller *m_controller;
    IStrategyListener *m_listener;
    Pending m_pending;
    uint64_t m_donateTicks;
    uint64_t m_target;
    uint64_t m_ticks;
};


} /* namespace xmrig */


#endif /* XMRIG_DONATESTRATEGY_H */
