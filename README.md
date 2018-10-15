# XMRig Proxy
:warning: **[Monero will change PoW algorithm on October 18](https://github.com/xmrig/xmrig/issues/753), all miners and proxy should be updated to [v2.8+](https://github.com/xmrig/xmrig-proxy/releases/tag/v2.8.1)** :warning:

[![Github All Releases](https://img.shields.io/github/downloads/xmrig/xmrig-proxy/total.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub release](https://img.shields.io/github/release/xmrig/xmrig-proxy/all.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub Release Date](https://img.shields.io/github/release-date-pre/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/releases)
[![GitHub license](https://img.shields.io/github/license/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/xmrig/xmrig-proxy.svg)](https://github.com/xmrig/xmrig-proxy/network)

Extremely high performance Monero (XMR) Stratum protocol proxy, can easily handle over 100K connections on cheap $5 (1024 MB) virtual machine. Reduce number of pool connections up to 256 times, 100K workers become just 391 worker on pool side. Written on C++/libuv same as [XMRig](https://github.com/xmrig/xmrig) miner.

## Compatibility
:warning: :warning: :warning: **Nicehash support must be enabled on miner side, it mandatory.** :warning: :warning: :warning:

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
  -b, --bind=ADDR          bind to specified address, example "0.0.0.0:3333"
  -a, --algo=ALGO          cryptonight (default) or cryptonight-lite
  -m, --mode=MODE          proxy mode, nicehash (default) or simple
  -o, --url=URL            URL of mining server
  -O, --userpass=U:P       username:password pair for mining server
  -u, --user=USERNAME      username for mining server
  -p, --pass=PASSWORD      password for mining server
  -r, --retries=N          number of times to retry before switch to backup server (default: 1)
  -R, --retry-pause=N      time to pause between retries (default: 1 second)
      --custom-diff=N      override pool diff
      --reuse-timeout=N    timeout in seconds for reuse pool connections in simple mode
      --verbose            verbose output
      --user-agent=AGENT   set custom user-agent string for pool
      --no-color           disable colored output
      --no-workers         disable per worker statistics
      --variant            algorithm PoW variant
      --donate-level=N     donate level, default 2%
  -B, --background         run the miner in the background
  -c, --config=FILE        load a JSON-format configuration file
      --no-watch           disable configuration file watching
  -l, --log-file=FILE      log all output to a file
  -S, --syslog             use system log for output messages
  -A  --access-log-file=N  log all workers access to a file
      --api-port=N         port for the miner API
      --api-access-token=T use Bearer access token for API
      --api-worker-id=ID   custom worker-id for API
      --api-no-ipv6        disable IPv6 support for API
      --api-no-restricted  enable full remote access (only if API token set)
  -h, --help               display this help and exit
  -V, --version            output version information and exit
```

## Donations

Default donation fee is 2% can be reduced to 1% or disabled via `donate-level` option. Donation fee applies only if you use more than 256 miners.

* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`
* BTC: `1P7ujsXeX7GxQwHNnJsRMgAdNkFZmNVqJT`

## Release checksums
### SHA-256
```
1b92b4fe901cdc03bc4ce7da962be8f22c24ad5919192ab3ae5b0dc202a6451a xmrig-proxy-2.8.1-xenial-amd64.tar.gz/xmrig-proxy-2.8.1/xmrig-proxy
7adb4c0958e6463c6032d60d098cb7693fca3c1541c0d9bb8a7fbd1d7489b452 xmrig-proxy-2.8.1-xenial-amd64.tar.gz/xmrig-proxy-2.8.1/xmrig-proxy-notls
08572c9ebbb9b0d6e1e1a6bcbeee1ba4e5bde2e660dbaca21e943024d98de73b xmrig-proxy-2.8.1-win32/xmrig-proxy.exe
543a6719b9e77c5a3215d5ed1d7e0d093a5a5c9da1580e71fe5f8adea755e4c6 xmrig-proxy-2.8.1-win32/xmrig-proxy-notls.exe
b4ded37e2873b8aa2841103477b7db6f5b3aed3692b6d27aa374cbdc38a46faa xmrig-proxy-2.8.1-win64/xmrig-proxy.exe
2443bc327fd2cabc8a3f20a8a23bca062fbd2ee367b4888f40def1d6d6dbaeff xmrig-proxy-2.8.1-win64/xmrig-proxy-notls.exe
```

## Contacts
* support@xmrig.com
* [reddit](https://www.reddit.com/user/XMRig/)
* [twitter](https://twitter.com/xmrig_dev)
