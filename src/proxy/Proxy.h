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

#ifndef __PROXY_H__
#define __PROXY_H__


#include <vector>
#include <uv.h>


#include "common/interfaces/IControllerListener.h"
#include "proxy/CustomDiff.h"
#include "proxy/Stats.h"
#include "proxy/workers/Worker.h"


class AccessLog;
class Addr;
class ISplitter;
class Login;
class Miner;
class Miners;
class Options;
class ProxyDebug;
class Server;
class ShareLog;
class Url;
class Workers;


namespace xmrig {
    class Controller;
}


class Proxy : public xmrig::IControllerListener
{
public:
    Proxy(xmrig::Controller *controller);
    ~Proxy();

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
    void onConfigChanged(xmrig::Config *config, xmrig::Config *previousConfig) override;

private:
    constexpr static int kPrintInterval = 60;
    constexpr static int kGCInterval    = 60;

    bool isColors() const;
    void bind(const Addr &addr);
    void gc();
    void print();
    void tick();

    static void onTick(uv_timer_t *handle);
    static void onTimer(uv_timer_t *handle);

    AccessLog *m_accessLog;
    CustomDiff m_customDiff;
    ISplitter *m_splitter;
    Login *m_login;
    Miners *m_miners;
    ProxyDebug *m_debug;
    ShareLog *m_shareLog;
    Stats m_stats;
    std::vector<Server*> m_servers;
    uint64_t m_ticks;
    uv_timer_t m_timer;
    Workers *m_workers;
    xmrig::Controller *m_controller;
};


#endif /* __PROXY_H__ */
