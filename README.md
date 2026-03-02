# XMRig Proxy
[![Github All Releases](https://img.shields.io/github/downloads/xmrig/xmrig-proxy/total.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub release](https://img.shields.io/github/release/xmrig/xmrig-proxy/all.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub Release Date](https://img.shields.io/github/release-date-pre/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub license](https://img.shields.io/github/license/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/network)

This is an extremely high-performance proxy for the CryptoNote stratum protocol (including Monero and others).
It can efficiently manage over 100K connections on an inexpensive, low-memory virtual machine (with just 1024 MB of RAM).
The proxy significantly reduces the number of connections to the pool, decreasing 100,000 workers down to just 391 on the pool side.
The codebase is shared with the [XMRig](https://github.com/xmrig/xmrig) miner.

## Compatibility
Compatible with any pool and any miner that supports NiceHash.

## Why?
This proxy is designed to handle donation traffic from XMRig. No other solution works well with high connection and disconnection rates.

## Download
* Binary releases: https://github.com/xmrig/xmrig-proxy/releases
* Git tree: https://github.com/xmrig/xmrig-proxy.git
  * Clone with `git clone https://github.com/xmrig/xmrig-proxy.git` :hammer: [Build instructions](https://xmrig.com/docs/proxy).
  
## Usage
:boom: If you are using Linux and need to manage over **1000 connections**, you must [increase the limits on open files](https://github.com/xmrig/xmrig-proxy/wiki/Ubuntu-setup).
  
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
      --daemon-zmq-port         daemon's zmq-pub port number (only use it if daemon has it enabled)
      --daemon-poll-interval=N  daemon poll interval in milliseconds (default: 1000)
      --daemon-job-timeout=N    daemon job timeout in milliseconds (default: 15000)
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

The default donation fee is 2%, which can be reduced to 1% or completely disabled using the `donate-level` option. This fee applies only when you utilize more than 256 miners.

* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`

## Contacts
* support@xmrig.com
* [X](https://x.com/xmrig_dev)
