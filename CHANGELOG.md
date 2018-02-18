# v2.4.5
- [#109](https://github.com/xmrig/xmrig-proxy/issues/109) Hashrate reports now more detailed for low speed workers.
- [#200](https://github.com/xmrig/xmrig/issues/200) In some cases proxy was doesn't write log to stdout.

# v2.4.4
 - Added libmicrohttpd version to --version output.
 - Fixed bug in singal handler, in some cases proxy wasn't shutdown properly.
 - Fixed recent MSVC 2017 version detection.
 - Fixed in default `config.json` was missing option `colors`.
 - [#37](https://github.com/xmrig/xmrig-proxy/issues/37) Fixed ARM build.
 - [#70](https://github.com/xmrig/xmrig-proxy/issues/70) Now used kH/s instead of KH/s.
 
# v2.4.2
 - [#153](https://github.com/xmrig/xmrig/issues/153) Fixed issues with dwarfpool.com.

# v2.4.1
 - [#25](https://github.com/xmrig/xmrig-proxy/issues/25) Use 2 decimal places in API hashrate.
 - [#147](https://github.com/xmrig/xmrig/issues/147) Fixed comparability with monero-stratum.
 - Fixed OS X build.

# v2.4.0
 - New internal event based architecture to easily extend proxy features.
 - Added [HTTP API](https://github.com/xmrig/xmrig-proxy/wiki/API).
 - Added per worker statistics, available in [HTTP API](https://github.com/xmrig/xmrig-proxy/wiki/API) and terminal.
 - Added command line option `--no-workers` and config option `workers`.
 - Added option `access-log-file`, to write to file log information about connection/disconnection of miners.
 - Added limited support to override pool diff, global via option `custom-diff` or per worker `WORKER_ID+DIFF`.
 - Added option `coin`, set it to `aeon` if use proxy for AEON (cryptonight-lite).
 - Added donation, default 2% configurable down to 1% as promised before, no fee if you use only one pool connection (up to 256 workers).
 - [#19](https://github.com/xmrig/xmrig-proxy/issues/19) Use ratio instead of efficiency in connections report.
 - Optimized performance, stability and memory usage.
 - libjansson replaced to rapidjson.

# v2.3.0
- Added config file support.
- Added support for 32bit version.
- Added `--user-agent` option, to set custom user-agent string for pool. For example `cpuminer-multi/0.1`.
- Force reconnect if pool block miner IP address. helps switch to backup pool.
- Better error message when detected incompatible miner, copy original nicehash behavior.
- Fixed [terminal issues](https://github.com/xmrig/xmrig-proxy/issues/2#issuecomment-319914085) after exit on Linux and OS X.
- [#5](https://github.com/xmrig/xmrig-proxy/issues/5) Fixed OX X support.
- [#6](https://github.com/xmrig/xmrig-proxy/issues/6) Fixed `--no-color` option.

# v2.2.0
- First public release.
