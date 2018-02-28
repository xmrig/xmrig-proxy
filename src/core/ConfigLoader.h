/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2018 XMRig       <support@xmrig.com>
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

#ifndef __CONFIGLOADER_H__
#define __CONFIGLOADER_H__


#include <stdint.h>


#include "rapidjson/fwd.h"


struct option;


namespace xmrig {


class Config;
class ConfigWatcher;
class IWatcherListener;


class ConfigLoader
{
    friend class ConfigWatcher;

public:
    static Config *load(int argc, char **argv, IWatcherListener *listener);
    static void release();

private:
    static bool getJSON(const char *fileName, rapidjson::Document &doc);
    static bool parseArg(Config *config, int key, const char *arg);
    static bool parseArg(Config *config, int key, uint64_t arg);
    static bool parseBoolean(Config *config, int key, bool enable);
    static void parseConfig(Config *config, const char *fileName);
    static void parseJSON(Config *config, const struct option *option, const rapidjson::Value &object);
    static void showUsage(int status);
    static void showVersion(void);

    static ConfigWatcher *m_watcher;
};


} /* namespace xmrig */

#endif /* __CONFIGLOADER_H__ */
