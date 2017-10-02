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

#ifndef __APISTATE_H__
#define __APISTATE_H__


#include "proxy/StatsData.h"
#include "proxy/workers/Worker.h"
#include "rapidjson/fwd.h"


class Hashrate;


class ApiState
{
public:
    ApiState();
    ~ApiState();

    char *get(const char *url, int *status) const;
    void tick(const StatsData &data);
    void tick(const std::vector<Worker> &workers);

private:
    char *finalize(rapidjson::Document &doc) const;
    void genId();
    void getHashrate(rapidjson::Document &doc) const;
    void getIdentify(rapidjson::Document &doc) const;
    void getMiner(rapidjson::Document &doc) const;
    void getMinersSummary(rapidjson::Document &doc) const;
    void getResources(rapidjson::Document &doc) const;
    void getResults(rapidjson::Document &doc) const;
    void getWorkers(rapidjson::Document &doc) const;

    char m_id[17];
    char m_workerId[128];
    StatsData m_stats;
    std::vector<Worker> m_workers;
};

#endif /* __APISTATE_H__ */
