/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2025 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2025 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#pragma once

#ifdef _MSC_VER
#   include "getopt/getopt.h"
#else
#   include <getopt.h>
#endif

#include "base/kernel/interfaces/IConfig.h"


namespace xmrig {


static char const short_options[] = "c:khBp:Px:r:R:s:T:o:u:O:Vl:Sb:A:a:C:m:L:46";


static struct option const options[] = {
    { "access-log-file",   1, nullptr, IConfig::AccessLogFileKey  },
    { "algo",              1, nullptr, IConfig::AlgorithmKey      },
    { "coin",              1, nullptr, IConfig::CoinKey           },
    { "api-worker-id",     1, nullptr, IConfig::ApiWorkerIdKey    },
    { "api-id",            1, nullptr, IConfig::ApiIdKey          },
    { "http-enabled",      0, nullptr, IConfig::HttpEnabledKey    },
    { "http-host",         1, nullptr, IConfig::HttpHostKey       },
    { "http-access-token", 1, nullptr, IConfig::HttpAccessTokenKey},
    { "http-port",         1, nullptr, IConfig::HttpPort          },
    { "http-no-restricted",0, nullptr, IConfig::HttpRestrictedKey },
    { "daemon",            0, nullptr, IConfig::DaemonKey         },
    { "daemon-poll-interval", 1, nullptr, IConfig::DaemonPollKey  },
    { "daemon-job-timeout", 1, nullptr, IConfig::DaemonJobTimeoutKey },
    { "self-select",       1, nullptr, IConfig::SelfSelectKey     },
    { "submit-to-origin",  0, nullptr, IConfig::SubmitToOriginKey },
    { "daemon-zmq-port",   1, nullptr, IConfig::DaemonZMQPortKey  },
    { "background",        0, nullptr, IConfig::BackgroundKey     },
    { "bind",              1, nullptr, IConfig::BindKey           },
    { "config",            1, nullptr, IConfig::ConfigKey         },
    { "custom-diff",       1, nullptr, IConfig::CustomDiffKey     },
    { "custom-diff-stats", 0, nullptr, IConfig::CustomDiffStatsKey},
    { "debug",             0, nullptr, IConfig::DebugKey          },
    { "donate-level",      1, nullptr, IConfig::DonateLevelKey    },
    { "keepalive",         2, nullptr, IConfig::KeepAliveKey      },
    { "log-file",          1, nullptr, IConfig::LogFileKey        },
    { "no-color",          0, nullptr, IConfig::ColorKey          },
    { "no-workers",        0, nullptr, IConfig::WorkersKey        },
    { "workers",           1, nullptr, IConfig::WorkersAdvKey     },
    { "pass",              1, nullptr, IConfig::PasswordKey       },
    { "pool-coin",         1, nullptr, IConfig::PoolCoinKey       },
    { "retries",           1, nullptr, IConfig::RetriesKey        },
    { "retry-pause",       1, nullptr, IConfig::RetryPauseKey     },
    { "syslog",            0, nullptr, IConfig::SyslogKey         },
    { "url",               1, nullptr, IConfig::UrlKey            },
    { "user",              1, nullptr, IConfig::UserKey           },
    { "user-agent",        1, nullptr, IConfig::UserAgentKey      },
    { "userpass",          1, nullptr, IConfig::UserpassKey       },
    { "verbose",           0, nullptr, IConfig::VerboseKey        },
    { "reuse-timeout",     1, nullptr, IConfig::ReuseTimeoutKey   },
    { "mode",              1, nullptr, IConfig::ModeKey           },
    { "rig-id",            1, nullptr, IConfig::RigIdKey          },
    { "tls",               0, nullptr, IConfig::TlsKey            },
    { "tls-fingerprint",   1, nullptr, IConfig::FingerprintKey    },
    { "tls-bind",          1, nullptr, IConfig::TlsBindKey        },
    { "tls-cert",          1, nullptr, IConfig::TlsCertKey        },
    { "tls-cert-key",      1, nullptr, IConfig::TlsCertKeyKey     },
    { "tls-dhparam",       1, nullptr, IConfig::TlsDHparamKey     },
    { "tls-protocols",     1, nullptr, IConfig::TlsProtocolsKey   },
    { "tls-ciphers",       1, nullptr, IConfig::TlsCiphersKey     },
    { "tls-ciphersuites",  1, nullptr, IConfig::TlsCipherSuitesKey},
    { "tls-gen",           1, nullptr, IConfig::TlsGenKey         },
    { "no-algo-ext",       0, nullptr, IConfig::AlgoExtKey        },
    { "access-password",   1, nullptr, IConfig::ProxyPasswordKey  },
    { "login-file",        1, nullptr, IConfig::LoginFileKey      },
    { "data-dir",          1, nullptr, IConfig::DataDirKey        },
    { "ipv4",              0, nullptr, IConfig::DnsIPv4Key        },
    { "ipv6",              0, nullptr, IConfig::DnsIPv6Key        },
    { "dns-ttl",           1, nullptr, IConfig::DnsTtlKey         },
    { "spend-secret-key",  1, nullptr, IConfig::SpendSecretKey    },
    { nullptr,             0, nullptr, 0 }
};


} // namespace xmrig
