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

#ifndef XMRIG_JOBRESULT_H
#define XMRIG_JOBRESULT_H


#include <stdint.h>
#include <string.h>


#include "common/crypto/Algorithm.h"
#include "common/net/Id.h"


class JobResult
{
public:
    inline JobResult() :
        nonce(nullptr),
        result(nullptr),
        id(0),
        diff(0)
    {
    }

    JobResult(int64_t id, const char *jobId, const char *nonce, const char *result, const xmrig::Algorithm &algorithm);

    bool isCompatible(uint8_t fixedByte) const;
    bool isValid() const;

    inline uint64_t actualDiff() const { return m_actualDiff; }

    const char *nonce;
    const char *result;
    const int64_t id;
    uint32_t diff;
    xmrig::Algorithm algorithm;
    xmrig::Id jobId;

private:
    uint64_t m_actualDiff;
};

#endif /* XMRIG_JOBRESULT_H */
