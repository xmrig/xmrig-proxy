/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_SIMPLESPLITTER_H
#define XMRIG_SIMPLESPLITTER_H


#include <map>
#include <stdint.h>
#include <vector>


#include "proxy/splitters/Splitter.h"


namespace xmrig {

class Controller;
class LoginEvent;
class Miner;
class SimpleMapper;
class Stats;
class SubmitEvent;


class SimpleSplitter : public Splitter
{
public:
    SimpleSplitter(Controller *controller);
    ~SimpleSplitter() override;

protected:
    Upstreams upstreams() const override;
    void connect() override;
    void gc() override;
    void printConnections() override;
    void tick(uint64_t ticks) override;

#   ifdef APP_DEVEL
    void printState() override;
#   endif

    inline void onRejectedEvent(IEvent *) override {}
    void onConfigChanged(Config *config, Config *previousConfig) override;
    void onEvent(IEvent *event) override;

private:
    void login(LoginEvent *event);
    void remove(Miner *miner);
    void removeIdle(uint64_t id);
    void removeUpstream(uint64_t id);
    void stop(SimpleMapper *mapper);
    void submit(SubmitEvent *event);

    std::map<uint64_t, SimpleMapper *> m_idles;
    std::map<uint64_t, SimpleMapper *> m_upstreams;
    std::vector<SimpleMapper *> m_released;
    uint64_t m_reuseTimeout;
    uint64_t m_sequence;
};


} /* namespace xmrig */


#endif /* XMRIG_SIMPLESPLITTER_H */
