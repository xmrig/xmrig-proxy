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

#ifndef __NONCESPLITTER_H__
#define __NONCESPLITTER_H__


#include <vector>


#include "interfaces/ISplitter.h"


class NonceMapper;
class Options;


class NonceSplitter : public ISplitter
{
public:
    NonceSplitter(const Options *options, const char *agent);
    ~NonceSplitter();

protected:
    void connect() override;
    void gc() override;
    void login(Miner *miner, const LoginRequest &request) override;
    void printConnections() override;
    void remove(Miner *miner) override;
    void submit(Miner *miner, const JobResult &request) override;

#   ifdef APP_DEVEL
    void printState() override;
#   endif

private:
    const char *m_agent;
    const Options *m_options;
    std::vector<NonceMapper*> m_upstreams;
};


#endif /* __PROXY_H__ */
