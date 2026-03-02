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

#include "proxy/splitters/donate/DonateMapper.h"
#include "base/io/json/Json.h"
#include "3rdparty/rapidjson/document.h"
#include "base/net/stratum/Client.h"
#include "proxy/events/AcceptEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"


xmrig::DonateMapper::DonateMapper(uint64_t id, LoginEvent *event, const Pool &pool) :
    m_miner(event->miner()),
    m_id(id)
{
    const rapidjson::Value &algo = event->params["algo"];
    if (algo.IsArray()) {
        for (const rapidjson::Value &value : algo.GetArray()) {
            m_algorithms.emplace_back(value.GetString());
        }
    }

    m_miner->setRouteId(0);
    m_miner->setMapperId(static_cast<ssize_t>(id));
    m_miner->setExtension(Miner::EXT_NICEHASH, true);

    m_client = new Client(-1, event->miner()->agent(), this);
    m_client->connect(pool);
    m_client->setQuiet(true);
}


xmrig::DonateMapper::~DonateMapper()
{
    m_client->deleteLater();
}


void xmrig::DonateMapper::submit(SubmitEvent *event)
{
    if (!isActive()) {
        return event->setError(Error::BadGateway);
    }

    JobResult req = event->request;
    req.diff = m_diff;

    m_client->submit(req);
}


void xmrig::DonateMapper::onClose(IClient *client, int)
{
    m_active = false;

    client->disconnect();
    m_miner->close();
}


void xmrig::DonateMapper::onJobReceived(IClient *, const Job &job, const rapidjson::Value &params)
{
    if (!isActive()) {
        return;
    }

    m_diff = job.diff();
    m_miner->forwardJob(job, Json::getString(params, "algo"));
}


void xmrig::DonateMapper::onLogin(IClient *, rapidjson::Document &doc, rapidjson::Value &params)
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    rapidjson::Value algo(kArrayType);

    for (const String &a : m_algorithms) {
        algo.PushBack(a.toJSON(), allocator);
    }

    params.AddMember("algo", algo, allocator);
}


void xmrig::DonateMapper::onLoginSuccess(IClient *)
{
}


void xmrig::DonateMapper::onResultAccepted(IClient *, const SubmitResult &result, const char *error)
{
    AcceptEvent::start(m_id, m_miner, result, true, false, error);

    if (error) {
        m_miner->replyWithError(result.reqId, error);
    }
    else {
        m_miner->success(result.reqId, "OK");
    }
}


void xmrig::DonateMapper::onVerifyAlgorithm(const IClient *client, const Algorithm &algorithm, bool *ok)
{

}
