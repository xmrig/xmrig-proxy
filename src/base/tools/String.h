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

#ifndef XMRIG_STRING_H
#define XMRIG_STRING_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "rapidjson/fwd.h"


namespace xmrig {


/**
 * @brief Simple C string wrapper.
 *
 * 1. I know about std:string.
 * 2. For some reason I prefer don't use std:string in miner, eg because of file size of MSYS2 builds.
 * 3. nullptr and JSON conversion supported.
 */
class String
{
public:
    inline String() : m_data(nullptr)                       {}
    inline String(char *str) : m_data(str)                  {}
    inline String(const char *str) : m_data(nullptr)        { set(str); }
    inline String(const String &other) : m_data(nullptr)    { set(other.data()); }
    inline String(String &&other) : m_data(other.m_data)    { other.m_data = nullptr; }
    inline ~String()                                        { free(m_data); }


    inline bool isEqual(const char *str) const  { return (m_data != nullptr && str != nullptr && strcmp(m_data, str) == 0) || (m_data == nullptr && m_data == nullptr); }
    inline bool contains(const char *str) const { return strstr(m_data, str) != nullptr; }


    inline bool isEmpty() const          { return size() == 0; }
    inline bool isNull() const           { return m_data == nullptr; }
    inline const char *data() const      { return m_data; }
    inline size_t size() const           { return isNull() ? 0 : strlen(m_data); }


    inline bool operator!=(const char *str) const      { return !isEqual(str); }
    inline bool operator!=(const String &str) const    { return !isEqual(str.data()); }
    inline bool operator<(const String &str) const     { return strcmp(data(), str.data()) < 0; }
    inline bool operator==(const char *str) const      { return isEqual(str); }
    inline bool operator==(const String &str) const    { return isEqual(str.data()); }
    inline String &operator=(char *str)                { set(str); return *this; }
    inline String &operator=(const char *str)          { set(str); return *this; }
    inline String &operator=(const String &str)        { set(str.data()); return *this; }
    inline String &operator=(String &&str)             { m_data = str.m_data; str.m_data = nullptr; return *this; }

    rapidjson::Value toJSON() const;
    rapidjson::Value toJSON(rapidjson::Document &doc) const;

private:
    inline void set(const char *str)
    {
        free(m_data);

        m_data = str != nullptr ? strdup(str) : nullptr;
    }


    inline void set(char *str)
    {
        free(m_data);

        m_data = str;
    }


    char *m_data;
};


} /* namespace xmrig */


#endif /* XMRIG_STRING_H */
