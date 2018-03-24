/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include <limits.h>
#include <stdio.h>
#include <uv.h>


#ifndef XMRIG_NO_HTTPD
#   include <microhttpd.h>
#endif


#include "core/Config.h"
#include "core/ConfigLoader.h"
#include "core/ConfigLoader_static.h"
#include "core/ConfigWatcher.h"
#include "interfaces/IWatcherListener.h"
#include "net/Url.h"
#include "Platform.h"
#include "proxy/Addr.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"


xmrig::ConfigWatcher *xmrig::ConfigLoader::m_watcher = nullptr;
xmrig::IWatcherListener *xmrig::ConfigLoader::m_listener = nullptr;


bool xmrig::ConfigLoader::loadFromFile(xmrig::Config *config, const char *fileName)
{
    rapidjson::Document doc;
    if (!getJSON(fileName, doc)) {
        return false;
    }

    config->setFileName(fileName);

    return loadFromJSON(config, doc);
}


bool xmrig::ConfigLoader::loadFromJSON(xmrig::Config *config, const char *json)
{
    rapidjson::Document doc;
    doc.Parse(json);

    if (doc.HasParseError() || !doc.IsObject()) {
        return false;
    }

    return loadFromJSON(config, doc);
}


bool xmrig::ConfigLoader::loadFromJSON(xmrig::Config *config, const rapidjson::Document &doc)
{
    for (size_t i = 0; i < ARRAY_SIZE(config_options); i++) {
        parseJSON(config, &config_options[i], doc);
    }

    const rapidjson::Value &pools = doc["pools"];
    if (pools.IsArray()) {
        for (const rapidjson::Value &value : pools.GetArray()) {
            if (!value.IsObject()) {
                continue;
            }

            for (size_t i = 0; i < ARRAY_SIZE(pool_options); i++) {
                parseJSON(config, &pool_options[i], value);
            }
        }
    }

    const rapidjson::Value &bind = doc["bind"];
    if (bind.IsArray()) {
        for (const rapidjson::Value &value : bind.GetArray()) {
            if (!value.IsString()) {
                continue;
            }

            parseArg(config, 'b', value.GetString());
        }
    }

    const rapidjson::Value &api = doc["api"];
    if (api.IsObject()) {
        for (size_t i = 0; i < ARRAY_SIZE(api_options); i++) {
            parseJSON(config, &api_options[i], api);
        }
    }

    config->adjust();
    return config->isValid();
}


bool xmrig::ConfigLoader::reload(xmrig::Config *oldConfig, const char *json)
{
    xmrig::Config *config = new xmrig::Config();
    if (!loadFromJSON(config, json)) {
        delete config;

        return false;
    }

    config->setFileName(oldConfig->fileName());
    const bool saved = config->save();

    if (config->watch() && m_watcher && saved) {
        delete config;

        return true;
    }

    m_listener->onNewConfig(config);
    return true;
}


xmrig::Config *xmrig::ConfigLoader::load(int argc, char **argv, IWatcherListener *listener)
{
    m_listener = listener;

    xmrig::Config *config = new xmrig::Config();
    int key;

    while (1) {
        key = getopt_long(argc, argv, short_options, options, NULL);
        if (key < 0) {
            break;
        }

        if (!parseArg(config, key, optarg)) {
            delete config;
            return nullptr;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "%s: unsupported non-option argument '%s'\n", argv[0], argv[optind]);
        delete config;
        return nullptr;
    }

    if (!config->isValid()) {
        loadFromFile(config, Platform::defaultConfigName());
    }

    if (!config->isValid()) {
        fprintf(stderr, "No pool URL supplied. Exiting.\n");
        delete config;
        return nullptr;
    }

    if (config->m_addrs.empty()) {
        config->m_addrs.push_back(new Addr("0.0.0.0:3333"));
        config->m_addrs.push_back(new Addr("[::]:3333"));
    }

    if (config->watch()) {
        m_watcher = new xmrig::ConfigWatcher(config->fileName(), listener);
    }

    config->adjust();
    return config;
}


void xmrig::ConfigLoader::release()
{
    delete m_watcher;
}


bool xmrig::ConfigLoader::getJSON(const char *fileName, rapidjson::Document &doc)
{
    uv_fs_t req;
    const int fd = uv_fs_open(uv_default_loop(), &req, fileName, O_RDONLY, 0644, nullptr);
    if (fd < 0) {
        fprintf(stderr, "unable to open %s: %s\n", fileName, uv_strerror(fd));
        return false;
    }

    uv_fs_req_cleanup(&req);

    FILE *fp = fdopen(fd, "rb");
    char buf[8192];
    rapidjson::FileReadStream is(fp, buf, sizeof(buf));

    doc.ParseStream(is);

    uv_fs_close(uv_default_loop(), &req, fd, nullptr);
    uv_fs_req_cleanup(&req);

    if (doc.HasParseError()) {
        printf("%s<%d>: %s\n", fileName, (int) doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
        return false;
    }

    return doc.IsObject();
}


bool xmrig::ConfigLoader::parseArg(xmrig::Config *config, int key, const char *arg)
{
    switch (key) {
    case 'a': /* --algo */
        config->setAlgo(arg);
        break;

    case 'm': /* --mode */
        config->setMode(arg);
        break;

    case 'b': /* --bind */
        {
            Addr *addr = new Addr(arg);
            if (addr->isValid()) {
                config->m_addrs.push_back(addr);
            }
            else {
                delete addr;
            }
        }
        break;

    case 'O': /* --userpass */
        if (!config->m_pools.back()->setUserpass(arg)) {
            return false;
        }

        break;

    case 'o': /* --url */
        if (config->m_pools.size() > 1 || config->m_pools[0]->isValid()) {
            Url *url = new Url(arg);
            if (url->isValid()) {
                config->m_pools.push_back(url);
            }
            else {
                delete url;
            }
        }
        else {
            config->m_pools[0]->parse(arg);
        }

        if (!config->m_pools.back()->isValid()) {
            return false;
        }

        break;

    case 'u': /* --user */
        config->m_pools.back()->setUser(arg);
        break;

    case 'p': /* --pass */
        config->m_pools.back()->setPassword(arg);
        break;

    case 'C':
        config->m_pools.back()->setCoin(arg);
        break;

    case 'l': /* --log-file */
        free(config->m_logFile);
        config->m_logFile = strdup(arg);
        break;

    case 'A': /* --access-log-file **/
        free(config->m_accessLog);
        config->m_accessLog = strdup(arg);
        break;

    case 4001: /* --access-token */
        free(config->m_apiToken);
        config->m_apiToken = strdup(arg);
        break;

    case 4002: /* --worker-id */
        free(config->m_apiWorkerId);
        config->m_apiWorkerId = strdup(arg);
        break;

    case 'r':  /* --retries */
    case 'R':  /* --retry-pause */
    case 1010: /* --variant */
    case 1102: /* --custom-diff */
    case 4000: /* --api-port */
        return parseArg(config, key, strtol(arg, nullptr, 10));

    case 'B':  /* --background */
    case 'S':  /* --syslog */
    case 1100: /* --verbose */
    case 1101: /* --debug */
        return parseBoolean(config, key, true);

    case 1002: /* --no-color */
    case 1103: /* --no-workers */
    case 1105: /* --no-watch */
    case 4004: /* ----api-no-restricted */
    case 4003: /* --api-no-ipv6 */
        return parseBoolean(config, key, false);

    case 1003: /* --donate-level */
        if (strncmp(arg, "minemonero.pro", 14) == 0) {
            config->m_donateLevel = 0;
        }
        else {
            parseArg(config, key, strtol(arg, nullptr, 10));
        }
        break;

    case 1104: /* --coin */
        config->setCoin(arg);
        break;

    case 'V': /* --version */
        showVersion();
        return false;

    case 'h': /* --help */
        showUsage(0);
        return false;

    case 'c': /* --config */
        loadFromFile(config, arg);
        break;

    case 1008: /* --user-agent */
        free(config->m_userAgent);
        config->m_userAgent = strdup(arg);
        break;

    default:
        showUsage(1);
        return false;
    }

    return true;
}


bool xmrig::ConfigLoader::parseArg(xmrig::Config *config, int key, uint64_t arg)
{
    switch (key) {
    case 'r': /* --retries */
        if (arg < 1 || arg > 1000) {
            showUsage(1);
            return false;
        }

        config->m_retries = (int) arg;
        break;

    case 'R': /* --retry-pause */
        if (arg < 1 || arg > 3600) {
            showUsage(1);
            return false;
        }

        config->m_retryPause = (int) arg;
        break;

    case 1003: /* --donate-level */
        if ((int) arg < 0 || arg > 99) {
            return true;
        }

        config->m_donateLevel = (int) arg;
        break;

    case 1010: /* --variant */
        config->m_pools.back()->setVariant((int) arg);
        break;

    case 4000: /* --api-port */
        if (arg <= 65536) {
            config->m_apiPort = (int) arg;
        }
        break;

    case 1102: /* --custom-diff */
        if (arg >= 100 && arg < INT_MAX) {
            config->m_diff = arg;
        }
        break;

    case 1106: /* --reuse-timeout */
        config->m_reuseTimeout = (int) arg;
        break;

    default:
        break;
    }

    return true;
}


bool xmrig::ConfigLoader::parseBoolean(xmrig::Config *config, int key, bool enable)
{
    switch (key) {
    case 'B': /* --background */
        config->m_background = enable;
        break;

    case 'S': /* --syslog */
        config->m_syslog = enable;
        break;

    case 1002: /* --no-color */
        config->m_colors = enable;
        break;

    case 1100: /* --verbose */
        config->m_verbose = enable;
        break;

    case 1101: /* --debug */
        config->m_debug = enable;
        break;

    case 2000: /* colors */
        config->m_colors = enable;
        break;

    case 1103: /* workers */
        config->m_workers = enable;
        break;

    case 1105: /* watch */
        config->m_watch = enable;
        break;

    case 4003: /* ipv6 */
        config->m_apiIPv6 = enable;

    case 4004: /* restricted */
        config->m_apiRestricted = enable;

    default:
        break;
    }

    return true;
}


void xmrig::ConfigLoader::parseJSON(xmrig::Config *config, const struct option *option, const rapidjson::Value &object)
{
    if (!option->name || !object.HasMember(option->name)) {
        return;
    }

    const rapidjson::Value &value = object[option->name];

    if (option->has_arg && value.IsString()) {
        parseArg(config, option->val, value.GetString());
    }
    else if (option->has_arg && value.IsInt64()) {
        parseArg(config, option->val, value.GetUint64());
    }
    else if (!option->has_arg && value.IsBool()) {
        parseBoolean(config, option->val, value.IsTrue());
    }
}


void xmrig::ConfigLoader::showUsage(int status)
{
    if (status) {
        fprintf(stderr, "Try \"" APP_ID "\" --help' for more information.\n");
    }
    else {
        printf(usage);
    }
}


void xmrig::ConfigLoader::showVersion()
{
    printf(APP_NAME " " APP_VERSION "\n built on " __DATE__

#   if defined(__clang__)
    " with clang " __clang_version__);
#   elif defined(__GNUC__)
    " with GCC");
    printf(" %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#   elif defined(_MSC_VER)
    " with MSVC");
    printf(" %d", MSVC_VERSION);
#   else
    );
#   endif

    printf("\n features:"
#   if defined(__i386__) || defined(_M_IX86)
    " i386"
#   elif defined(__x86_64__) || defined(_M_AMD64)
    " x86_64"
#   endif

#   if defined(__AES__) || defined(_MSC_VER)
    " AES-NI"
#   endif
    "\n");

    printf("\nlibuv/%s\n", uv_version_string());

#   ifndef XMRIG_NO_HTTPD
    printf("libmicrohttpd/%s\n", MHD_get_version());
#   endif
}
