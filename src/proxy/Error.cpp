/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "proxy/Error.h"


namespace xmrig {

static const char *kBadGateway            = "Bad gateway";
static const char *kInvalidJobId          = "Invalid job id";
static const char *kInvalidMethod         = "Invalid method";
static const char *kInvalidNonce          = "Invalid nonce; is miner not compatible with NiceHash?";
static const char *kLowDifficulty         = "Low difficulty share";
static const char *kUnauthenticated       = "Unauthenticated";
static const char *kUnknownError          = "Unknown error";
static const char *kIncompatibleAlgorithm = "No compatible algorithm found, change algo option in your miner.";
static const char *kIncorrectAlgorithm    = "Incorrect algorithm";
static const char *kForbidden             = "Permission denied";
static const char *kRouteNotFound         = "Algorithm negotiation failed";

} /* namespace xmrig */


const char *xmrig::Error::toString(int code)
{
    switch (code)
    {
    case BadGateway:
        return kBadGateway;

    case InvalidJobId:
        return kInvalidJobId;

    case InvalidMethod:
        return kInvalidMethod;

    case InvalidNonce:
        return kInvalidNonce;

    case LowDifficulty:
        return kLowDifficulty;

    case Unauthenticated:
        return kUnauthenticated;

    case IncompatibleAlgorithm:
        return kIncompatibleAlgorithm;

    case IncorrectAlgorithm:
        return kIncorrectAlgorithm;

    case Forbidden:
        return kForbidden;

    case RouteNotFound:
        return kRouteNotFound;

    default:
        break;
    }

    return kUnknownError;
}
