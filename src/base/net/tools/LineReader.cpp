/* XMRig
 * Copyright (c) 2020      cohcho      <https://github.com/cohcho>
 * Copyright (c) 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "base/net/tools/LineReader.h"
#include "base/kernel/interfaces/ILineListener.h"

#include <cassert>
#include <cstring>


void xmrig::LineReader::parse(char *data, size_t size)
{
    assert(m_listener != nullptr && size > 0);
    if (!m_listener || size == 0) {
        return;
    }

    getline(data, size);
}


void xmrig::LineReader::reset()
{
    std::vector<char>().swap(m_buf);
    m_discard = false;
}


void xmrig::LineReader::add(const char *data, size_t size)
{
    if (m_discard) {
        return;
    }

    if (m_buf.size() > m_maxSize || size > m_maxSize - m_buf.size()) {
        std::vector<char>().swap(m_buf);
        m_discard = true;
        return;
    }

    m_buf.insert(m_buf.end(), data, data + size);
}


void xmrig::LineReader::getline(char *data, size_t size)
{
    char *end        = nullptr;
    char *start      = data;
    size_t remaining = size;

    while ((end = static_cast<char*>(memchr(start, '\n', remaining))) != nullptr) {
        *end = '\0';

        end++;

        const auto len = static_cast<size_t>(end - start);
        if (m_discard) {
            reset();
        }
        else if (!m_buf.empty()) {
            add(start, len - 1);
            if (!m_discard) {
                // The size limit excludes the delimiter, just as for an unfragmented line.
                m_buf.push_back('\0');
                m_listener->onLine(m_buf.data(), m_buf.size() - 1);
            }
            reset();
        }
        else if (len > 1 && len - 1 <= m_maxSize) {
            m_listener->onLine(start, len - 1);
        }

        remaining -= len;
        start = end;
    }

    if (remaining > 0) {
        add(start, remaining);
    }
}
