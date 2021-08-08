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

#ifndef XMRIG_PROXY_H
#define XMRIG_PROXY_H


#include <vector>


#include "base/kernel/interfaces/IBaseListener.h"
#include "base/kernel/interfaces/ITimerListener.h"
#include "base/tools/Object.h"
#include "proxy/CustomDiff.h"
#include "proxy/Stats.h"
#include "proxy/workers/Worker.h"


namespace xmrig {


class AccessLog;
class ApiRouter;
class BindHost;
class Controller;
class DonateSplitter;
class ISplitter;
class Login;
class Miner;
class Miners;
class ProxyDebug;
class Server;
class ShareLog;
class TlsContext;
class Workers;


class Proxy : public IBaseListener, public ITimerListener
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(Proxy)

    Proxy(Controller *controller);
    ~Proxy() override;

    void connect();
    void printConnections();
    void printHashrate();
    void printWorkers();
    void toggleDebug();

    const StatsData &statsData() const;
    const std::vector<Worker> &workers() const;
    std::vector<Miner*> miners() const;

#   ifdef APP_DEVEL
    void printState();
#   endif

protected:
    inline void onTimer(const Timer *) override { tick(); }

    void onConfigChanged(Config *config, Config *previousConfig) override;

private:
    constexpr static int kGCInterval    = 60;

    void bind(const BindHost &host);
    void gc();
    void print();
    void tick();

    AccessLog *m_accessLog;
    ApiRouter *m_api    = nullptr;
    Controller *m_controller;
    CustomDiff m_customDiff;
    DonateSplitter *m_donate;
    ISplitter *m_splitter;
    Login *m_login;
    Miners *m_miners;
    ProxyDebug *m_debug;
    ShareLog *m_shareLog;
    Stats *m_stats;
    std::vector<Server*> m_servers;
    Timer *m_timer      = nullptr;
    TlsContext *m_tls   = nullptr;
    uint64_t m_ticks    = 0;
    Workers *m_workers;
};


} /* namespace xmrig */


#endif /* XMRIG_PROXY_H */
