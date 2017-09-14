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

#ifndef __TICKINGCOUNTER_H__
#define __TICKINGCOUNTER_H__


#include <stdint.h>
#include <stdio.h>
#include <vector>


template <class T> class TickingCounter
{
public:
    inline TickingCounter(size_t tickTime) :
        m_tickTime(tickTime),
        m_pending(0)
    {}


    inline double calc(size_t seconds) const
    {
        const size_t ticks = seconds / m_tickTime;
        const size_t size  = m_data.size();

        uint64_t count = 0;
        for (size_t i = size < ticks ? 0 : (size - ticks); i < size; ++i) {
            count += m_data[i];
        }

        if (count == 0) {
            return 0.0;
        }

        return (double) count / (ticks * m_tickTime * 1000);
    }


    inline size_t tickTime() const { return m_tickTime; }
    inline void add(T count)       { m_pending += count; }
    inline void tick()             { m_data.push_back(m_pending); m_pending = 0; }

private:
    size_t m_tickTime;
    std::vector<T> m_data;
    T m_pending;
};


#endif /* __TICKINGCOUNTER_H__ */
