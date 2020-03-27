/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2014-2019 heapwolf    <https://github.com/heapwolf>
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


#ifndef XMRIG_HTTPCONTEXT_H
#define XMRIG_HTTPCONTEXT_H


typedef struct http_parser http_parser;
typedef struct http_parser_settings http_parser_settings;
typedef struct uv_connect_s uv_connect_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_tcp_s uv_tcp_t;


#include "base/net/http/HttpRequest.h"


namespace xmrig {


class IHttpListener;


class HttpContext : public HttpRequest
{
public:
    HttpContext(int parser_type, IHttpListener *listener);
    ~HttpContext();

    inline uv_stream_t *stream() const { return reinterpret_cast<uv_stream_t *>(tcp); }
    inline uv_handle_t *handle() const { return reinterpret_cast<uv_handle_t *>(tcp); }

    void close();

    static HttpContext *get(uint64_t id);
    static void attach(http_parser_settings *settings);
    static void closeAll();

    http_parser *parser;
    IHttpListener *listener;
    uv_connect_t *connect;
    uv_tcp_t *tcp;

private:
    static int onHeaderField(http_parser *parser, const char *at, size_t length);
    static int onHeaderValue(http_parser *parser, const char *at, size_t length);

    void setHeader();

    bool m_wasHeaderValue;
    std::string m_lastHeaderField;
    std::string m_lastHeaderValue;

    static std::map<uint64_t, HttpContext *> m_storage;
};


} // namespace xmrig


#endif // XMRIG_HTTPCONTEXT_H

