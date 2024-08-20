# XMRig Proxy
[![Github All Releases](https://img.shields.io/github/downloads/xmrig/xmrig-proxy/total.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub release](https://img.shields.io/github/release/xmrig/xmrig-proxy/all.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub Release Date](https://img.shields.io/github/release-date-pre/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub license](https://img.shields.io/github/license/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/network)

Extremely high performance Monero (XMR) Stratum protocol proxy, can easily handle over 100K connections on cheap $5 (1024 MB) virtual machine. Reduce number of pool connections up to 256 times, 100K workers become just 391 worker on pool side. Written on C++/libuv same as [XMRig](https://github.com/xmrig/xmrig) miner.

## Compatibility
:warning: :warning: :warning: **Nicehash support must be enabled on miner side, it is mandatory.** :warning: :warning: :warning:

* Compatible with any Monero, Electroneum, Sumokoin and AEON pools, except **nicehash.com**.
* Any miner with nicehash support, `--nicehash` option for [XMRig](https://github.com/xmrig/xmrig), `"nicehash_nonce": true,` for xmr-stak-cpu.
* [Comparison](https://github.com/xmrig/xmrig-proxy/wiki/Comparison) with other proxies.

## Why?
This proxy designed and created for handle donation traffic from XMRig. No one other solution works fine with high connection/disconnection rate.

## Download
* Binary releases: https://github.com/xmrig/xmrig-proxy/releases
* Git tree: https://github.com/xmrig/xmrig-proxy.git
  * Clone with `git clone https://github.com/xmrig/xmrig-proxy.git` :hammer: [Build instructions](https://github.com/xmrig/xmrig-proxy/wiki/Build).
  
## Usage
:boom: If you use Linux and want handle more than **1000 connections**, you need [increase limits of open files](https://github.com/xmrig/xmrig-proxy/wiki/Ubuntu-setup).

Use [config.xmrig.com](https://config.xmrig.com/proxy) to generate, edit or share configurations.
  
### Options
```
Network:
  -o, --url=URL                 URL of mining server
  -a, --algo=ALGO               mining algorithm https://xmrig.com/docs/algorithms
      --coin=COIN               specify coin instead of algorithm
  -u, --user=USERNAME           username for mining server
  -p, --pass=PASSWORD           password for mining server
  -O, --userpass=U:P            username:password pair for mining server
  -x, --proxy=HOST:PORT         connect through a SOCKS5 proxy
  -k, --keepalive               send keepalived packet for prevent timeout (needs pool support)
      --rig-id=ID               rig identifier for pool-side statistics (needs pool support)
      --tls                     enable SSL/TLS support (needs pool support)
      --tls-fingerprint=HEX     pool TLS certificate fingerprint for strict certificate pinning
      --dns-ipv6                prefer IPv6 records from DNS responses
      --dns-ttl=N               N seconds (default: 30) TTL for internal DNS cache
      --daemon                  use daemon RPC instead of pool for solo mining
      --daemon-poll-interval=N  daemon poll interval in milliseconds (default: 1000)
      --self-select=URL         self-select block templates from URL
      --submit-to-origin        also submit solution back to self-select URL
  -r, --retries=N               number of times to retry before switch to backup server (default: 5)
  -R, --retry-pause=N           time to pause between retries (default: 5)
      --user-agent              set custom user-agent string for pool
      --donate-level=N          donate level, default 0%%

Options:
  -b, --bind=ADDR               bind to specified address, example "0.0.0.0:3333"
  -m, --mode=MODE               proxy mode, nicehash (default) or simple
      --custom-diff=N           override pool diff
      --custom-diff-stats       calculate stats using custom diff shares instead of pool shares
      --reuse-timeout=N         timeout in seconds for reuse pool connections in simple mode
      --no-workers              disable per worker statistics
      --access-password=P       set password to restrict connections to the proxy
      --no-algo-ext             disable "algo" protocol extension

API:
      --api-worker-id=ID        custom worker-id for API
      --api-id=ID               custom instance ID for API
      --http-host=HOST          bind host for HTTP API (default: 127.0.0.1)
      --http-port=N             bind port for HTTP API
      --http-access-token=T     access token for HTTP API
      --http-no-restricted      enable full remote access to HTTP API (only if access token set)

TLS:
      --tls-bind=ADDR           bind to specified address with enabled TLS
      --tls-gen=HOSTNAME        generate TLS certificate for specific hostname
      --tls-cert=FILE           load TLS certificate chain from a file in the PEM format
      --tls-cert-key=FILE       load TLS certificate private key from a file in the PEM format
      --tls-dhparam=FILE        load DH parameters for DHE ciphers from a file in the PEM format
      --tls-protocols=N         enable specified TLS protocols, example: "TLSv1 TLSv1.1 TLSv1.2 TLSv1.3"
      --tls-ciphers=S           set list of available ciphers (TLSv1.2 and below)
      --tls-ciphersuites=S      set list of available TLSv1.3 ciphersuites

Logging:
  -S, --syslog                  use system log for output messages
  -l, --log-file=FILE           log all output to a file
  -A  --access-log-file=FILE    log all workers access to a file
      --no-color                disable colored output
      --verbose                 verbose output

Misc:
  -c, --config=FILE             load a JSON-format configuration file
  -B, --background              run the proxy in the background
  -V, --version                 output version information and exit
  -h, --help                    display this help and exit
      --dry-run                 test configuration and exit
```

## Donations

Default donation fee is 2% can be reduced to 1% or disabled via `donate-level` option. Donation fee applies only if you use more than 256 miners.

* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`

## Contacts
* support@xmrig.com
* [reddit](https://www.reddit.com/user/XMRig/)
* [twitter](https://twitter.com/xmrig_dev)
