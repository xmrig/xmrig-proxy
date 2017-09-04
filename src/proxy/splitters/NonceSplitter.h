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

#ifndef __NONCESPLITTER_H__
#define __NONCESPLITTER_H__


#include <uv.h>
#include <vector>


#include "interfaces/IEventListener.h"


class LoginEvent;
class NonceMapper;
class Options;
class SubmitEvent;


class NonceSplitter : public IEventListener
{
public:
    NonceSplitter(const Options *options, const char *agent);
    ~NonceSplitter();

    void connect();
    void gc();
    void printConnections();

#   ifdef APP_DEVEL
    void printState();
#   endif

protected:
    void onEvent(IEvent *event) override;
    inline void onRejectedEvent(IEvent *event) override {}

private:
    constexpr static int kTickInterval = 1 * 1000;

    static void onTick(uv_timer_t *handle);

    void login(LoginEvent *event);
    void remove(Miner *miner);
    void submit(SubmitEvent *event);
    void tick();

    const char *m_agent;
    const Options *m_options;
    std::vector<NonceMapper*> m_upstreams;
    uv_timer_t m_timer;
};


#endif /* __PROXY_H__ */
