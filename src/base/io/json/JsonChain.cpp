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


#include "base/io/json/Json.h"
#include "base/io/json/JsonChain.h"
#include "base/io/log/Log.h"
#include "rapidjson/error/en.h"


namespace xmrig {

static const rapidjson::Value kNullValue;

}


xmrig::JsonChain::JsonChain()
{
}


bool xmrig::JsonChain::add(rapidjson::Document &&doc)
{
    if (doc.HasParseError() || !doc.IsObject() || doc.ObjectEmpty()) {
        return false;
    }

    m_chain.push_back(std::move(doc));

    return true;
}


bool xmrig::JsonChain::addFile(const char *fileName)
{
    using namespace rapidjson;
    Document doc;
    if (Json::get(fileName, doc)) {
        m_fileName = fileName;

        return add(std::move(doc));
    }

    if (doc.HasParseError()) {
        LOG_ERR("%s<offset:%zu>: \"%s\"", fileName, doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
    }
    else {
        LOG_ERR("unable to open \"%s\".", fileName);
    }

    return false;
}


bool xmrig::JsonChain::addRaw(const char *json)
{
    using namespace rapidjson;
    Document doc;
    doc.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(json);

    return add(std::move(doc));
}


void xmrig::JsonChain::dump(const char *fileName)
{
    rapidjson::Document doc(rapidjson::kArrayType);

    for (rapidjson::Document &value : m_chain) {
        doc.PushBack(value, doc.GetAllocator());
    }

    Json::save(fileName, doc);
}


bool xmrig::JsonChain::getBool(const char *key, bool defaultValue) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsBool()) {
            return i->value.GetBool();
        }
    }

    return defaultValue;
}


const char *xmrig::JsonChain::getString(const char *key, const char *defaultValue) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsString()) {
            return i->value.GetString();
        }
    }

    return defaultValue;
}


const rapidjson::Value &xmrig::JsonChain::getArray(const char *key) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsArray()) {
            return i->value;
        }
    }

    return kNullValue;
}


const rapidjson::Value &xmrig::JsonChain::getObject(const char *key) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsObject()) {
            return i->value;
        }
    }

    return kNullValue;
}


const rapidjson::Value &xmrig::JsonChain::getValue(const char *key) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd()) {
            return i->value;
        }
    }

    return kNullValue;
}


int xmrig::JsonChain::getInt(const char *key, int defaultValue) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsInt()) {
            return i->value.GetInt();
        }
    }

    return defaultValue;
}


int64_t xmrig::JsonChain::getInt64(const char *key, int64_t defaultValue) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsInt64()) {
            return i->value.GetInt64();
        }
    }

    return defaultValue;
}


uint64_t xmrig::JsonChain::getUint64(const char *key, uint64_t defaultValue) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsUint64()) {
            return i->value.GetUint64();
        }
    }

    return defaultValue;
}


unsigned xmrig::JsonChain::getUint(const char *key, unsigned defaultValue) const
{
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it) {
        auto i = it->FindMember(key);
        if (i != it->MemberEnd() && i->value.IsUint()) {
            return i->value.GetUint();
        }
    }

    return defaultValue;
}
