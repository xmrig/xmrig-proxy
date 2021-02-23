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

#ifndef XMRIG_USAGE_H
#define XMRIG_USAGE_H


#include "version.h"


#include <string>


namespace xmrig {


static char const usage_raw[] = "\
Usage: " APP_ID " [OPTIONS]\n\
Options:\n\
  -b, --bind=ADDR           bind to specified address, example \"0.0.0.0:3333\"\n\
  -a, --algo=ALGO           mining algorithm https://xmrig.com/docs/algorithms\n\
      --coin=COIN           specify coin instead of algorithm\n\
  -m, --mode=MODE           proxy mode, nicehash (default) or simple\n\
  -o, --url=URL             URL of mining server\n\
  -O, --userpass=U:P        username:password pair for mining server\n\
  -u, --user=USERNAME       username for mining server\n\
  -p, --pass=PASSWORD       password for mining server\n\
      --rig-id=ID           rig identifier for pool-side statistics (needs pool support)\n\
      --tls-fingerprint=F   pool TLS certificate fingerprint, if set enable strict certificate pinning\n\
  -k, --keepalive           prevent timeout (needs pool support)\n\
  -r, --retries=N           number of times to retry before switch to backup server (default: 1)\n\
  -R, --retry-pause=N       time to pause between retries (default: 1 second)\n\
      --custom-diff=N       override pool diff\n\
      --custom-diff-stats   calculate stats using custom diff shares instead of pool shares\n\
      --reuse-timeout=N     timeout in seconds for reuse pool connections in simple mode\n\
      --algo-perf-same-threshold=N     algo perf threshold in percent that proxy uses to group miners into one upstream\n\
      --verbose             verbose output\n\
      --user-agent=AGENT    set custom user-agent string for pool\n\
      --no-color            disable colored output\n\
      --no-workers          disable per worker statistics\n\
      --variant             algorithm PoW variant\n\
      --donate-level=N      donate level, default 2%%\n\
  -B, --background          run the miner in the background\n\
  -c, --config=FILE         load a JSON-format configuration file\n\
  -l, --log-file=FILE       log all output to a file\n"
#ifdef HAVE_SYSLOG_H
"\
  -S, --syslog              use system log for output messages\n"
#endif
"\
  -A  --access-log-file=N   log all workers access to a file\n\
      --access-password=P   set password to restrict connections to the proxy\n\
      --no-algo-ext         disable \"algo\" protocol extension\n\
      --api-worker-id=ID    custom worker-id (instance name) for API\n\
      --api-id=ID           custom instance ID for API\n\
      --http-enabled        enable HTTP API\n\
      --http-host=HOST      bind host for HTTP API (by default 127.0.0.1)\n\
      --http-port=N         bind port for HTTP API\n\
      --http-access-token=T access token for HTTP API\n\
      --http-no-restricted  enable full remote access to HTTP API (only if access token set)\n"
#ifdef XMRIG_FEATURE_TLS
"\
      --tls                 enable SSL/TLS support for pool connection (needs pool support)\n\
      --tls-gen=HOSTNAME    generate TLS certificate for specific hostname\n\
      --tls-bind=ADDR       bind to specified address with enabled TLS\n\
      --tls-cert=FILE       load TLS certificate chain from a file in the PEM format\n\
      --tls-cert-key=FILE   load TLS certificate private key from a file in the PEM format\n\
      --tls-dhparam=FILE    load DH parameters for DHE ciphers from a file in the PEM format\n\
      --tls-protocols=N     enable specified TLS protocols, example: \"TLSv1 TLSv1.1 TLSv1.2 TLSv1.3\"\n\
      --tls-ciphers=S       set list of available ciphers (TLSv1.2 and below)\n\
      --tls-ciphersuites=S  set list of available TLSv1.3 ciphersuites\n"
#endif
"\
  -h, --help                display this help and exit\n\
  -V, --version             output version information and exit\n\
";


static inline const std::string &usage()
{
    static std::string u;

    if (u.empty()) {
        u = usage_raw;
    }

    return u;
}


} /* namespace xmrig */

#endif /* XMRIG_USAGE_H */
