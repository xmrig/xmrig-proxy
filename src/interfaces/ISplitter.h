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

#ifndef __ISPLITTER_H__
#define __ISPLITTER_H__


class JobResult;
class LoginRequest;
class Miner;


class ISplitter
{
public:
    virtual ~ISplitter() {}

    virtual void connect() = 0;
    virtual void gc() = 0;
    virtual void login(Miner *miner, const LoginRequest &request) = 0;
    virtual void printConnections() = 0;
    virtual void remove(Miner *miner) = 0;
    virtual void submit(Miner *miner, const JobResult &request) = 0;

#   ifdef APP_DEVEL
    virtual void printState() = 0;
#   endif
};


#endif // __IWORKER_H__
