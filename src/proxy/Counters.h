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

#ifndef __COUNTERS_H__
#define __COUNTERS_H__


#include <stdint.h>


class Counters
{
public:
    static inline void reset()
    {
        m_added   = 0;
        m_removed = 0;
        accepted  = 0;
    }


    static inline void add()
    {
        m_miners++;
        m_added++;

        if (m_miners > m_maxMiners) {
            m_maxMiners = m_miners;
        }
    }


    static inline void remove()
    {
        m_miners--;
        m_removed++;
    }


    static inline uint32_t added()     { return m_added; }
    static inline uint32_t removed()   { return m_removed; }
    static inline uint64_t maxMiners() { return m_maxMiners; }
    static inline uint64_t miners()    { return m_miners; }

    static uint64_t accepted;
    static uint64_t connections;
    static uint64_t expired;

private:
    static uint32_t m_added;
    static uint32_t m_removed;
    static uint64_t m_maxMiners;
    static uint64_t m_miners;
};

#endif /* __COUNTERS_H__ */
