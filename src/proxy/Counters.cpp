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


#include <inttypes.h>


#include "Counters.h"
#include "log/Log.h"
#include "Options.h"


uint64_t Counters::m_counters[3]  = { 0 };
uint64_t Counters::m_minersMax    = 0;
uv_timer_t Counters::m_timer;

Counters::Tick Counters::tick;


//void Counters::accept(Counters::Stores store, size_t id, uint32_t diff, uint64_t ms, bool verbose)
//{
//    m_hashrate[store].accepted++;
//    m_hashrate[store].add(diff);

//    tick.accepted++;

//    if (verbose) {
//        LOG_INFO(Options::i()->colors() ? "#%03u \x1B[01;32maccepted\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff \x1B[01;37m%u\x1B[0m \x1B[01;30m(%" PRIu64 " ms)"
//                                        : "#%03u accepted (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff %u (%" PRIu64 " ms)"  ,
//                 id, m_hashrate[0].accepted, m_hashrate[0].rejected[0], m_hashrate[0].rejected[1], diff, ms);
//    }
//}


void Counters::add(CounterTypes type)
{
    m_counters[type]++;
}


//void Counters::reject(Stores store, const char *ip, const char *message)
//{
//    m_hashrate[store].rejected[1]++;
////    LOG_INFO("[% -15s] \x1B[01;31mrejected\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") \x1B[31m\"%s\"\x1B[0m",
////             ip, m_hashrate[0].accepted, m_hashrate[0].rejected[0], m_hashrate[0].rejected[1], message);
//}


//void Counters::reject(Stores store, size_t id, uint32_t diff, uint64_t ms, const char *error)
//{
//    m_hashrate[store].rejected[0]++;
//    LOG_INFO(Options::i()->colors() ? "#%03u \x1B[01;31mrejected\x1B[0m (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff \x1B[01;37m%u\x1B[0m \x1B[31m\"%s\"\x1B[0m \x1B[01;30m(%" PRId64 " ms)"
//                                    : "#%03u rejected (%" PRId64 "/%" PRId64 "+%" PRId64 ") diff %u \"%s\" (%" PRId64 " ms)",
//             id, m_hashrate[0].accepted, m_hashrate[0].rejected[0], m_hashrate[0].rejected[1], diff, error, ms);
//}


void Counters::remove(CounterTypes type)
{
    m_counters[type]--;
}


void Counters::start()
{
    uv_timer_init(uv_default_loop(), &m_timer);
    uv_timer_start(&m_timer, Counters::onTick, Hashrate::kTickTime * 1000, Hashrate::kTickTime * 1000);
}


void Counters::onTick(uv_timer_t *handle)
{
//    m_hashrate[0].tick();
//    m_hashrate[1].tick();
}



