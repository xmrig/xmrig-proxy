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

#ifndef XMRIG_WORKERS_H
#define XMRIG_WORKERS_H


#include <map>
#include <string>
#include <vector>


#include "3rdparty/rapidjson/fwd.h"
#include "base/kernel/interfaces/IBaseListener.h"
#include "proxy/interfaces/IEventListener.h"
#include "proxy/workers/Worker.h"


namespace xmrig {


class AcceptEvent;
class CloseEvent;
class Controller;
class LoginEvent;
class Miner;
class SubmitEvent;


class Workers : public IEventListener, public IBaseListener
{
public:
    enum Mode {
        None,     // workers support disabled.
        RigID,    // rig_id with failback to user, default since 2.6.0
        User,
        Password,
        Agent,
        IP
    };

    Workers(Controller *controller);
    ~Workers() override;

    void printWorkers();
    void reset();
    void tick(uint64_t ticks);

    inline const std::vector<Worker> &workers() const { return m_workers; }
    inline Mode mode() const                          { return m_mode; }

    static const char *modeName(Mode mode);
    static Mode parseMode(const char *mode);
    static rapidjson::Value modeToJSON(Mode mode);

protected:
    void onConfigChanged(Config *config, Config *previousConfig) override;
    void onEvent(IEvent *event) override;
    void onRejectedEvent(IEvent *event) override;

private:
    inline bool isEnabled() const { return m_mode != None; }

    bool indexByMiner(const Miner *miner, size_t *index) const;
    const char *nameByMiner(const Miner *miner) const;
    size_t add(const Miner *miner);
    void accept(const AcceptEvent *event);
    void login(const LoginEvent *event);
    void reject(const SubmitEvent *event);
    void remove(const CloseEvent *event);

    Controller *m_controller;
    Mode m_mode;
    std::map<int64_t, size_t> m_miners;
    std::map<std::string, size_t> m_map;
    std::vector<Worker> m_workers;
};


} /* namespace xmrig */


#endif /* XMRIG_WORKERS_H */
