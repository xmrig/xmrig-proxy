/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#ifndef XMRIG_DONATESPLITTER_H
#define XMRIG_DONATESPLITTER_H


#include <map>


#include "base/kernel/EventListener.h"


namespace xmrig {


class Controller;
class DonateMapper;
class LoginEvent;
class Miner;
class SubmitEvent;


class DonateSplitter : public EventListener
{
public:
    DonateSplitter(Controller *controller);

protected:
    void onEvent(uint32_t type, IEvent *event) override;

private:
    void login(LoginEvent *event);
    void reject(LoginEvent *event);
    void remove(Miner *miner);
    void remove(size_t id);
    void submit(SubmitEvent *event);

    Controller *m_controller;
    std::map<uint64_t, DonateMapper *> m_mappers;
    uint64_t m_sequence = 0;
};


} // namespace xmrig


#endif // XMRIG_DONATESPLITTER_H
