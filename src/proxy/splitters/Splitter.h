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

#ifndef XMRIG_SPLITTER_H
#define XMRIG_SPLITTER_H


#include "base/kernel/EventListener.h"
#include "proxy/interfaces/ISplitter.h"


namespace xmrig {


class Controller;


class Splitter : public EventListener, public ISplitter
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(Splitter)

    Splitter(Controller *controller) : m_controller(controller) {}
    ~Splitter() = default;

protected:
    Controller *m_controller;
};


} // namespace xmrig


#endif // XMRIG_SPLITTER_H
