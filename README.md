# XMRig Proxy

Extremely high performance Monero (XMR) Stratum protocol proxy, can easily handle over 100K connections on cheap $5 (1024 MB) virtual machine. Reduce number of pool connections up to 256 times, 100K workers become just 291 worker on pool side. Written on C++/libuv same as [XMRig](https://github.com/xmrig/xmrig) miner.

## Compatibility

* Compatible with any Monero and AEON pools, strongly recommended use pool with fixed diff feature.
* Any miner with nicehash support, `--nicehash` option for [XMRig](https://github.com/xmrig/xmrig).

## Download
* Binary releases: https://github.com/xmrig/xmrig-proxy/releases
* Git tree: https://github.com/xmrig/xmrig-proxy.git
  * Clone with `git clone https://github.com/xmrig/xmrig-proxy.git` [Build instructions](https://github.com/xmrig/xmrig-proxy/wiki/Build).
  
## Usage
### Basic example
```
xmrig-proxy.exe -o pool.minemonero.pro:5555 -u YOUR_WALLET -p x --bind 0.0.0.0:3333 --bind 0.0.0.0:5555 
```

### Failover
```
xmrig-proxy.exe -o pool.minemonero.pro:5555 -u YOUR_WALLET1 -o pool.supportxmr.com:5555 -u YOUR_WALLET2 -p x --bind 0.0.0.0:5555 
```
For failover you can add multiple pools, maximum count not limited.
  
### Options
```
  -b, --bind=ADDR       bind to specified address, example "0.0.0.0:3333"
  -o, --url=URL         URL of mining server
  -O, --userpass=U:P    username:password pair for mining server
  -u, --user=USERNAME   username for mining server
  -p, --pass=PASSWORD   password for mining server
  -k, --keepalive       send keepalived for prevent timeout (need pool support)
  -r, --retries=N       number of times to retry before switch to backup server (default: 5)
  -R, --retry-pause=N   time to pause between retries (default: 5)
      --no-color        disable colored output
      --verbose         verbose output
  -B, --background      run the miner in the background
  -l, --log-file=FILE   log all output to a file
  -h, --help            display this help and exit
  -V, --version         output version information and exit
```

## Donations
Proxy at this moment does not contain any developer fee. It may added in future or not, anyway it will always free if you had less than 256 workers.

* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`
* BTC: `1P7ujsXeX7GxQwHNnJsRMgAdNkFZmNVqJT`

## Contacts
* support@xmrig.com
* [reddit](https://www.reddit.com/user/XMRig/)
