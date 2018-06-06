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


#include <stdio.h>


#include "common/net/Job.h"
#include "net/JobResult.h"


JobResult::JobResult(int64_t id, const char *jobId, const char *nonce, const char *result, const xmrig::Algorithm &algorithm) :
    nonce(nonce),
    result(result),
    id(id),
    diff(0),
    algorithm(algorithm),
    jobId(jobId, 3),
    m_actualDiff(0)
{
}


bool JobResult::isCompatible(uint8_t fixedByte) const
{
    uint8_t n[4];
    if (!Job::fromHex(nonce, 8, n)) {
        return false;
    }

    return n[3] == fixedByte;
}


bool JobResult::isValid() const
{
    if (!nonce || !result) {
        return false;
    }

    return strlen(nonce) == 8 && jobId.isValid() && strlen(result) == 64;
}


uint64_t JobResult::actualDiff() const
{
    if (!m_actualDiff) {
        uint8_t data[32];
        Job::fromHex(result, sizeof(data) * 2, data);

        m_actualDiff = Job::toDiff(reinterpret_cast<const uint64_t*>(data)[3]);
    }

    return m_actualDiff;
}
