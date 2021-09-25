/* XMRig
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

#ifndef XMRIG_SIMPLESPLITTER_H
#define XMRIG_SIMPLESPLITTER_H


#include <map>
#include <vector>


#include "3rdparty/rapidjson/fwd.h"
#include "proxy/splitters/Splitter.h"


namespace xmrig {


class ConfigEvent;
class Controller;
class LoginEvent;
class Miner;
class SimpleMapper;
class SubmitEvent;


class SimpleSplitter : public Splitter
{
public:
    SimpleSplitter(Controller *controller, const ConfigEvent *event);
    ~SimpleSplitter() override;

protected:
    Upstreams upstreams() const override;
    void connect() override;
    void gc() override;
    void printConnections() override;
    void tick(uint64_t ticks, uint64_t now) override;

#   ifdef APP_DEVEL
    void printState() override;
#   endif

    void onEvent(uint32_t type, IEvent *event) override;

private:
    void login(const LoginEvent *event);
    void remove(Miner *miner);
    void removeIdle(uint64_t id);
    void removeUpstream(uint64_t id);
    void save(rapidjson::Document &doc) const;
    void stop(SimpleMapper *mapper);
    void submit(SubmitEvent *event);

    std::map<uint64_t, SimpleMapper *> m_idles;
    std::map<uint64_t, SimpleMapper *> m_upstreams;
    std::vector<SimpleMapper *> m_released;
    uint64_t m_reuseTimeout;
    uint64_t m_sequence = 0;
};


} // namespace xmrig


#endif // XMRIG_SIMPLESPLITTER_H
