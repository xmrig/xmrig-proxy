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

#ifndef __CONFIGLOADER_STATIC_H__
#define __CONFIGLOADER_STATIC_H__


#ifdef _MSC_VER
#   include "getopt/getopt.h"
#else
#   include <getopt.h>
#endif


#include "version.h"


#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif


namespace xmrig {


static char const usage[] = "\
Usage: " APP_ID " [OPTIONS]\n\
Options:\n\
  -b, --bind=ADDR          bind to specified address, example \"0.0.0.0:3333\"\n\
  -a, --algo=ALGO          cryptonight (default) or cryptonight-lite\n\
  -m, --mode=MODE          proxy mode, nicehash (default) or simple\n\
  -o, --url=URL            URL of mining server\n\
  -O, --userpass=U:P       username:password pair for mining server\n\
  -u, --user=USERNAME      username for mining server\n\
  -p, --pass=PASSWORD      password for mining server\n\
  -k, --keepalive          prevent timeout (need pool support)\n\
  -r, --retries=N          number of times to retry before switch to backup server (default: 1)\n\
  -R, --retry-pause=N      time to pause between retries (default: 1 second)\n\
      --custom-diff=N      override pool diff\n\
      --reuse-timeout=N    timeout in seconds for reuse pool connections in simple mode\n\
      --verbose            verbose output\n\
      --user-agent=AGENT   set custom user-agent string for pool\n\
      --no-color           disable colored output\n\
      --no-workers         disable per worker statistics\n\
      --variant            algorithm PoW variant\n\
      --donate-level=N     donate level, default 2%%\n\
  -B, --background         run the miner in the background\n\
  -c, --config=FILE        load a JSON-format configuration file\n\
      --no-watch           disable configuration file watching\n\
  -l, --log-file=FILE      log all output to a file\n"
# ifdef HAVE_SYSLOG_H
"\
  -S, --syslog             use system log for output messages\n"
# endif
"\
  -A  --access-log-file=N  log all workers access to a file\n\
      --api-port=N         port for the miner API\n\
      --api-access-token=T use Bearer access token for API\n\
      --api-worker-id=ID   custom worker-id for API\n\
      --api-no-ipv6        disable IPv6 support for API\n\
      --api-no-restricted  enable full remote access (only if API token set)\n\
  -h, --help               display this help and exit\n\
  -V, --version            output version information and exit\n\
";


static char const short_options[] = "c:khBp:Px:r:R:s:T:o:u:O:Vl:Sb:A:a:C:m:";


static struct option const options[] = {
    { "access-log-file",   1, nullptr, 'A'  },
    { "algo",              1, nullptr, 'a'  },
    { "api-access-token",  1, nullptr, 4001 },
    { "api-no-ipv6",       0, nullptr, 4003 },
    { "api-no-restricted", 0, nullptr, 4004 },
    { "api-port",          1, nullptr, 4000 },
    { "api-worker-id",     1, nullptr, 4002 },
    { "background",        0, nullptr, 'B'  },
    { "bind",              1, nullptr, 'b'  },
    { "coin",              1, nullptr, 1104 },
    { "config",            1, nullptr, 'c'  },
    { "custom-diff",       1, nullptr, 1102 },
    { "debug",             0, nullptr, 1101 },
    { "donate-level",      1, nullptr, 1003 },
    { "help",              0, nullptr, 'h'  },
    { "keepalive",         0, nullptr ,'k'  },
    { "log-file",          1, nullptr, 'l'  },
    { "no-color",          0, nullptr, 1002 },
    { "no-watch",          0, nullptr, 1105 },
    { "no-workers",        0, nullptr, 1103 },
    { "pass",              1, nullptr, 'p'  },
    { "pool-coin",         1, nullptr, 'C'  },
    { "retries",           1, nullptr, 'r'  },
    { "retry-pause",       1, nullptr, 'R'  },
    { "syslog",            0, nullptr, 'S'  },
    { "url",               1, nullptr, 'o'  },
    { "user",              1, nullptr, 'u'  },
    { "user-agent",        1, nullptr, 1008 },
    { "userpass",          1, nullptr, 'O'  },
    { "verbose",           0, nullptr, 1100 },
    { "version",           0, nullptr, 'V'  },
    { "variant",           1, nullptr, 1010 },
    { "reuse-timeout",     1, nullptr, 1106 },
    { "mode",              1, nullptr, 'm'  },
    { 0, 0, 0, 0 }
};


static struct option const config_options[] = {
    { "access-log-file",  1, nullptr, 'A'  },
    { "algo",             1, nullptr, 'a'  },
    { "background",       0, nullptr, 'B'  },
    { "coin",             1, nullptr, 1104 },
    { "colors",           0, nullptr, 2000 },
    { "custom-diff",      1, nullptr, 1102 },
    { "debug",            0, nullptr, 1101 },
    { "donate-level",     1, nullptr, 1003 },
    { "log-file",         1, nullptr, 'l'  },
    { "retries",          1, nullptr, 'r'  },
    { "retry-pause",      1, nullptr, 'R'  },
    { "syslog",           0, nullptr, 'S'  },
    { "user-agent",       1, nullptr, 1008 },
    { "verbose",          0, nullptr, 1100 },
    { "watch",            0, nullptr, 1105 },
    { "workers",          0, nullptr, 1103 },
    { "reuse-timeout",    1, nullptr, 1106 },
    { "mode",             1, nullptr, 'm'  },
    { 0, 0, 0, 0 }
};


static struct option const pool_options[] = {
    { "url",           1, nullptr, 'o'  },
    { "pass",          1, nullptr, 'p'  },
    { "user",          1, nullptr, 'u'  },
    { "userpass",      1, nullptr, 'O'  },
    { "coin",          1, nullptr, 'C'  },
    { "keepalive",     2, nullptr ,'k'  },
    { "variant",       1, nullptr, 1010 },
    { 0, 0, 0, 0 }
};


static struct option const api_options[] = {
    { "port",          1, nullptr, 4000 },
    { "access-token",  1, nullptr, 4001 },
    { "worker-id",     1, nullptr, 4002 },
    { "ipv6",          0, nullptr, 4003 },
    { "restricted",    0, nullptr, 4004 },
    { 0, 0, 0, 0 }
};


} /* namespace xmrig */

#endif /* __CONFIGLOADER_STATIC_H__ */
