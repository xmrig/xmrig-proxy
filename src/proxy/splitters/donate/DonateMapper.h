/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_DONATEMAPPER_H
#define XMRIG_DONATEMAPPER_H


#include <cstdint>
#include <vector>


#include "base/kernel/interfaces/IClientListener.h"
#include "base/tools/Object.h"
#include "base/tools/String.h"


namespace xmrig {


class LoginEvent;
class Miner;
class Pool;
class SubmitEvent;


class DonateMapper : public IClientListener
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(DonateMapper)

    DonateMapper(uint64_t id, LoginEvent *event, const Pool &pool);
    ~DonateMapper() override;

    inline bool isActive() const { return m_active; }
    inline uint64_t id() const   { return m_id; }

    void submit(SubmitEvent *event);

protected:
    void onClose(IClient *client, int failures) override;
    void onJobReceived(IClient *client, const Job &job, const rapidjson::Value &params) override;
    void onLogin(IClient *client, rapidjson::Document &doc, rapidjson::Value &params) override;
    void onLoginSuccess(IClient *client) override;
    void onResultAccepted(IClient *client, const SubmitResult &result, const char *error) override;
    void onVerifyAlgorithm(const IClient *client, const Algorithm &algorithm, bool *ok) override;

private:
    bool m_active = true;
    IClient *m_client;
    Miner *m_miner;
    std::vector<String> m_algorithms;
    uint64_t m_diff = 0;
    uint64_t m_id;
};


} /* namespace xmrig */


#endif /* XMRIG_DONATEMAPPER_H */
