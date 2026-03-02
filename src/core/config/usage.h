/* XMRig
 * Copyright (c) 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright (c) 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright (c) 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright (c) 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright (c) 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright (c) 2018-2025 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2025 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include "version.h"

#include <string>


namespace xmrig {


static inline const std::string &usage()
{
    static std::string u;

    if (!u.empty()) {
        return u;
    }

    u += "Usage: " APP_ID " [OPTIONS]\n\nNetwork:\n";

    u += "  -o, --url=URL                 URL of mining server\n";
    u += "  -a, --algo=ALGO               mining algorithm https://xmrig.com/docs/algorithms\n";
    u += "      --coin=COIN               specify coin instead of algorithm\n";
    u += "  -u, --user=USERNAME           username for mining server\n";
    u += "  -p, --pass=PASSWORD           password for mining server\n";
    u += "  -O, --userpass=U:P            username:password pair for mining server\n";
    u += "  -x, --proxy=HOST:PORT         connect through a SOCKS5 proxy\n";
    u += "  -k, --keepalive               send keepalived packet for prevent timeout (needs pool support)\n";
    u += "      --rig-id=ID               rig identifier for pool-side statistics (needs pool support)\n";

#   ifdef XMRIG_FEATURE_TLS
    u += "      --tls                     enable SSL/TLS support (needs pool support)\n";
    u += "      --tls-fingerprint=HEX     pool TLS certificate fingerprint for strict certificate pinning\n";
#   endif

    u += "  -4, --ipv4                    resolve names to IPv4 addresses\n";
    u += "  -6, --ipv6                    resolve names to IPv6 addresses\n";
    u += "      --dns-ttl=N               N seconds (default: 30) TTL for internal DNS cache\n";

#   ifdef XMRIG_FEATURE_HTTP
    u += "      --daemon                  use daemon RPC instead of pool for solo mining\n";
    u += "      --daemon-zmq-port         daemon's zmq-pub port number (only use it if daemon has it enabled)\n";
    u += "      --daemon-poll-interval=N  daemon poll interval in milliseconds (default: 1000)\n";
    u += "      --daemon-job-timeout=N    daemon job timeout in milliseconds (default: 15000)\n";
    u += "      --self-select=URL         self-select block templates from URL\n";
    u += "      --submit-to-origin        also submit solution back to self-select URL\n";
#   endif

    u += "  -r, --retries=N               number of times to retry before switch to backup server (default: 5)\n";
    u += "  -R, --retry-pause=N           time to pause between retries (default: 5)\n";
    u += "      --user-agent              set custom user-agent string for pool\n";
    u += "      --donate-level=N          donate level, default 0%%\n";

    u += "\nOptions:\n";
    u += "  -b, --bind=ADDR               bind to specified address, example \"0.0.0.0:3333\"\n";
    u += "  -m, --mode=MODE               proxy mode, nicehash (default) or simple\n";
    u += "      --custom-diff=N           override pool diff\n";
    u += "      --custom-diff-stats       calculate stats using custom diff shares instead of pool shares\n";
    u += "      --reuse-timeout=N         timeout in seconds for reuse pool connections in simple mode\n";
    u += "      --no-workers              disable per worker statistics\n";
    u += "      --access-password=P       set password to restrict connections to the proxy\n";
    u += "      --no-algo-ext             disable \"algo\" protocol extension\n";

#   ifdef XMRIG_FEATURE_HTTP
    u += "\nAPI:\n";
    u += "      --api-worker-id=ID        custom worker-id for API\n";
    u += "      --api-id=ID               custom instance ID for API\n";
    u += "      --http-host=HOST          bind host for HTTP API (default: 127.0.0.1)\n";
    u += "      --http-port=N             bind port for HTTP API\n";
    u += "      --http-access-token=T     access token for HTTP API\n";
    u += "      --http-no-restricted      enable full remote access to HTTP API (only if access token set)\n";
#   endif

#   ifdef XMRIG_FEATURE_TLS
    u += "\nTLS:\n";
    u += "      --tls-bind=ADDR           bind to specified address with enabled TLS\n";
    u += "      --tls-gen=HOSTNAME        generate TLS certificate for specific hostname\n";
    u += "      --tls-cert=FILE           load TLS certificate chain from a file in the PEM format\n";
    u += "      --tls-cert-key=FILE       load TLS certificate private key from a file in the PEM format\n";
    u += "      --tls-dhparam=FILE        load DH parameters for DHE ciphers from a file in the PEM format\n";
    u += "      --tls-protocols=N         enable specified TLS protocols, example: \"TLSv1 TLSv1.1 TLSv1.2 TLSv1.3\"\n";
    u += "      --tls-ciphers=S           set list of available ciphers (TLSv1.2 and below)\n";
    u += "      --tls-ciphersuites=S      set list of available TLSv1.3 ciphersuites\n";
#   endif

    u += "\nLogging:\n";
#   ifdef HAVE_SYSLOG_H
    u += "  -S, --syslog                  use system log for output messages\n";
#   endif

    u += "  -l, --log-file=FILE           log all output to a file\n";
    u += "  -A  --access-log-file=FILE    log all workers access to a file\n";
    u += "      --no-color                disable colored output\n";
    u += "      --verbose                 verbose output\n";

    u += "\nMisc:\n";
    u += "  -c, --config=FILE             load a JSON-format configuration file\n";
    u += "  -B, --background              run the proxy in the background\n";
    u += "  -V, --version                 output version information and exit\n";
    u += "  -h, --help                    display this help and exit\n";
    u += "      --dry-run                 test configuration and exit\n";

    return u;
}


} // namespace xmrig
