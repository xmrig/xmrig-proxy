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

#ifndef XMRIG_JOBRESULT_H
#define XMRIG_JOBRESULT_H


#include <cstdint>
#include <cstring>


#include "base/crypto/Algorithm.h"
#include "base/tools/String.h"


namespace xmrig {


class JobResult
{
public:
    static constexpr uint32_t backend = 0;

    JobResult() = default;
    JobResult(int64_t id, const char *jobId, const char *nonce, const char *result, const xmrig::Algorithm &algorithm, const char* sig, const char* sig_data, uint8_t view_tag, int64_t extra_nonce);

    bool isCompatible(uint8_t fixedByte) const;
    bool isValid() const;

    inline uint64_t actualDiff() const { return m_actualDiff; }

    Algorithm algorithm;
    const char *nonce         = nullptr;
    const char *result        = nullptr;
    const char *sig           = nullptr;
    const char *sig_data      = nullptr;
    const uint8_t view_tag    = 0;
    const int64_t id          = 0;
    const int64_t extra_nonce = -1;
    String jobId;
    uint64_t diff             = 0;

private:
    uint64_t m_actualDiff     = 0;
};


} /* namespace xmrig */


#endif /* XMRIG_JOBRESULT_H */
