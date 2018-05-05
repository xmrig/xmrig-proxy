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

#ifndef __WORKERS_H__
#define __WORKERS_H__


#include <map>
#include <string>
#include <vector>


#include "interfaces/IEventListener.h"
#include "proxy/workers/Worker.h"


class AcceptEvent;
class CloseEvent;
class LoginEvent;
class Miner;
class SubmitEvent;


namespace xmrig {
    class Controller;
}


class Workers : public IEventListener
{
public:
    Workers(xmrig::Controller *controller);
    ~Workers();

    void printWorkers();
    void tick(uint64_t ticks);

    inline const std::vector<Worker> &workers() const { return m_workers; }

protected:
    void onEvent(IEvent *event) override;
    void onRejectedEvent(IEvent *event) override;

private:
    bool indexByMiner(const Miner *miner, size_t *index) const;
    void accept(const AcceptEvent *event);
    void login(const LoginEvent *event);
    void reject(const SubmitEvent *event);
    void remove(const CloseEvent *event);

    bool m_enabled;
    std::map<int64_t, size_t> m_miners;
    std::map<std::string, size_t> m_map;
    std::vector<Worker> m_workers;
    xmrig::Controller *m_controller;
};


#endif /* __WORKERS_H__ */
