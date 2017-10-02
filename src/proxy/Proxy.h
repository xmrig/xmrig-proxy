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

#ifndef __PROXY_H__
#define __PROXY_H__


#include <vector>
#include <uv.h>


#include "proxy/CustomDiff.h"
#include "proxy/Stats.h"


class AccessLog;
class Miners;
class NonceSplitter;
class Options;
class ProxyDebug;
class Server;
class ShareLog;
class Url;
class Workers;


class Proxy
{
public:
    Proxy(const Options *options);
    ~Proxy();

    void connect();
    void printConnections();
    void printHashrate();
    void printWorkers();
    void toggleDebug();

#   ifdef APP_DEVEL
    void printState();
#   endif

private:
    constexpr static int kPrintInterval = 60;
    constexpr static int kGCInterval    = 60;

    void bind(const char *ip, uint16_t port);
    void gc();
    void print();
    void tick();

    static void onTick(uv_timer_t *handle);
    static void onTimer(uv_timer_t *handle);

    AccessLog *m_accessLog;
    CustomDiff m_customDiff;
    Miners *m_miners;
    NonceSplitter *m_splitter;
    ProxyDebug *m_debug;
    ShareLog *m_shareLog;
    Stats m_stats;
    std::vector<Server*> m_servers;
    uint64_t m_ticks;
    uv_timer_t m_timer;
    Workers *m_workers;
};


#endif /* __PROXY_H__ */
