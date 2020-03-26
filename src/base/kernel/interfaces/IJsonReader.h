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

#ifndef XMRIG_IJSONREADER_H
#define XMRIG_IJSONREADER_H


#include "rapidjson/fwd.h"


namespace xmrig {


class IJsonReader
{
public:
    virtual ~IJsonReader() = default;

    virtual bool getBool(const char *key, bool defaultValue = false) const                       = 0;
    virtual bool isEmpty() const                                                                 = 0;
    virtual const char *getString(const char *key, const char *defaultValue = nullptr) const     = 0;
    virtual const rapidjson::Value &getArray(const char *key) const                              = 0;
    virtual const rapidjson::Value &getObject(const char *key) const                             = 0;
    virtual const rapidjson::Value &getValue(const char *key) const                              = 0;
    virtual int getInt(const char *key, int defaultValue = 0) const                              = 0;
    virtual int64_t getInt64(const char *key, int64_t defaultValue = 0) const                    = 0;
    virtual uint64_t getUint64(const char *key, uint64_t defaultValue = 0) const                 = 0;
    virtual unsigned getUint(const char *key, unsigned defaultValue = 0) const                   = 0;
};


} /* namespace xmrig */


#endif // XMRIG_IJSONREADER_H
