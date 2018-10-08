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

#ifndef XMRIG_CONFIGLOADER_PLATFORM_H
#define XMRIG_CONFIGLOADER_PLATFORM_H


#ifdef _MSC_VER
#   include "getopt/getopt.h"
#else
#   include <getopt.h>
#endif


#include "common/interfaces/IConfig.h"
#include "version.h"


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
      --rig-id=ID          rig identifier for pool-side statistics (needs pool support)\n\
      --tls                enable SSL/TLS support (needs pool support)\n\
      --tls-fingerprint=F  pool TLS certificate fingerprint, if set enable strict certificate pinning\n\
  -k, --keepalive          prevent timeout (needs pool support)\n\
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
      --api-worker-id=ID   custom worker-id (instance name) for API\n\
      --api-id=ID          custom instance ID for API\n\
      --api-ipv6           enable IPv6 support for API\n\
      --api-no-restricted  enable full remote access (only if API token set)\n\
  -h, --help               display this help and exit\n\
  -V, --version            output version information and exit\n\
";


static char const short_options[] = "c:khBp:Px:r:R:s:T:o:u:O:Vl:Sb:A:a:C:m:";


static struct option const options[] = {
    { "access-log-file",   1, nullptr, xmrig::IConfig::AccessLogFileKey  },
    { "algo",              1, nullptr, xmrig::IConfig::AlgorithmKey      },
    { "api-access-token",  1, nullptr, xmrig::IConfig::ApiAccessTokenKey },
    { "api-ipv6",          0, nullptr, xmrig::IConfig::ApiIPv6Key        },
    { "api-no-restricted", 0, nullptr, xmrig::IConfig::ApiRestrictedKey  },
    { "api-port",          1, nullptr, xmrig::IConfig::ApiPort           },
    { "api-worker-id",     1, nullptr, xmrig::IConfig::ApiWorkerIdKey    },
    { "api-id",            1, nullptr, xmrig::IConfig::ApiIdKey          },
    { "background",        0, nullptr, xmrig::IConfig::BackgroundKey     },
    { "bind",              1, nullptr, xmrig::IConfig::BindKey           },
    { "coin",              1, nullptr, xmrig::IConfig::CoinKey           },
    { "config",            1, nullptr, xmrig::IConfig::ConfigKey         },
    { "custom-diff",       1, nullptr, xmrig::IConfig::CustomDiffKey     },
    { "debug",             0, nullptr, xmrig::IConfig::DebugKey          },
    { "donate-level",      1, nullptr, xmrig::IConfig::DonateLevelKey    },
    { "help",              0, nullptr, xmrig::IConfig::HelpKey           },
    { "keepalive",         2, nullptr, xmrig::IConfig::KeepAliveKey      },
    { "log-file",          1, nullptr, xmrig::IConfig::LogFileKey        },
    { "no-color",          0, nullptr, xmrig::IConfig::ColorKey          },
    { "no-watch",          0, nullptr, xmrig::IConfig::WatchKey          },
    { "no-workers",        0, nullptr, xmrig::IConfig::WorkersKey        },
    { "workers",           1, nullptr, xmrig::IConfig::WorkersAdvKey     },
    { "pass",              1, nullptr, xmrig::IConfig::PasswordKey       },
    { "pool-coin",         1, nullptr, xmrig::IConfig::PoolCoinKey       },
    { "retries",           1, nullptr, xmrig::IConfig::RetriesKey        },
    { "retry-pause",       1, nullptr, xmrig::IConfig::RetryPauseKey     },
    { "syslog",            0, nullptr, xmrig::IConfig::SyslogKey         },
    { "url",               1, nullptr, xmrig::IConfig::UrlKey            },
    { "user",              1, nullptr, xmrig::IConfig::UserKey           },
    { "user-agent",        1, nullptr, xmrig::IConfig::UserAgentKey      },
    { "userpass",          1, nullptr, xmrig::IConfig::UserpassKey       },
    { "verbose",           0, nullptr, xmrig::IConfig::VerboseKey        },
    { "version",           0, nullptr, xmrig::IConfig::VersionKey        },
    { "variant",           1, nullptr, xmrig::IConfig::VariantKey        },
    { "reuse-timeout",     1, nullptr, xmrig::IConfig::ReuseTimeoutKey   },
    { "mode",              1, nullptr, xmrig::IConfig::ModeKey           },
    { "rig-id",            1, nullptr, xmrig::IConfig::RigIdKey          },
    { "tls",               0, nullptr, xmrig::IConfig::TlsKey            },
    { "tls-fingerprint",   1, nullptr, xmrig::IConfig::FingerprintKey    },
    { nullptr,             0, nullptr, 0 }
};


static struct option const config_options[] = {
    { "access-log-file",  1, nullptr, xmrig::IConfig::AccessLogFileKey  },
    { "algo",             1, nullptr, xmrig::IConfig::AlgorithmKey      },
    { "background",       0, nullptr, xmrig::IConfig::BackgroundKey     },
    { "coin",             1, nullptr, xmrig::IConfig::CoinKey           },
    { "colors",           0, nullptr, xmrig::IConfig::ColorKey          },
    { "custom-diff",      1, nullptr, xmrig::IConfig::CustomDiffKey     },
    { "debug",            0, nullptr, xmrig::IConfig::DebugKey          },
    { "donate-level",     1, nullptr, xmrig::IConfig::DonateLevelKey    },
    { "log-file",         1, nullptr, xmrig::IConfig::LogFileKey        },
    { "retries",          1, nullptr, xmrig::IConfig::RetriesKey        },
    { "retry-pause",      1, nullptr, xmrig::IConfig::RetryPauseKey     },
    { "syslog",           0, nullptr, xmrig::IConfig::SyslogKey         },
    { "user-agent",       1, nullptr, xmrig::IConfig::UserAgentKey      },
    { "verbose",          0, nullptr, xmrig::IConfig::VerboseKey        },
    { "watch",            0, nullptr, xmrig::IConfig::WatchKey          },
    { "workers",          1, nullptr, xmrig::IConfig::WorkersAdvKey     },
    { "reuse-timeout",    1, nullptr, xmrig::IConfig::ReuseTimeoutKey   },
    { "mode",             1, nullptr, xmrig::IConfig::ModeKey           },
    { nullptr,            0, nullptr, 0 }
};


static struct option const pool_options[] = {
    { "url",              1, nullptr, xmrig::IConfig::UrlKey         },
    { "pass",             1, nullptr, xmrig::IConfig::PasswordKey    },
    { "user",             1, nullptr, xmrig::IConfig::UserKey        },
    { "userpass",         1, nullptr, xmrig::IConfig::UserpassKey    },
    { "keepalive",        2, nullptr, xmrig::IConfig::KeepAliveKey   },
    { "variant",          1, nullptr, xmrig::IConfig::VariantKey     },
    { "rig-id",           1, nullptr, xmrig::IConfig::RigIdKey       },
    { "tls",              0, nullptr, xmrig::IConfig::TlsKey         },
    { "tls-fingerprint",  1, nullptr, xmrig::IConfig::FingerprintKey },
    { nullptr,            0, nullptr, 0 }
};


static struct option const api_options[] = {
    { "port",          1, nullptr, xmrig::IConfig::ApiPort           },
    { "access-token",  1, nullptr, xmrig::IConfig::ApiAccessTokenKey },
    { "worker-id",     1, nullptr, xmrig::IConfig::ApiWorkerIdKey    },
    { "ipv6",          0, nullptr, xmrig::IConfig::ApiIPv6Key        },
    { "restricted",    0, nullptr, xmrig::IConfig::ApiRestrictedKey  },
    { "id",            1, nullptr, xmrig::IConfig::ApiIdKey          },
    { nullptr,         0, nullptr, 0 }
};


} /* namespace xmrig */

#endif /* XMRIG_CONFIGLOADER_PLATFORM_H */
