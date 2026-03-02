# v2.99.1-beta
- [#1066](https://github.com/xmrig/xmrig/issues/1066#issuecomment-518080529) Added error message if pool not ready for RandomX.
- Name for reference RandomX configuration changed to `rx/text` to avoid potential conflicts in future.

# v2.99.0-beta
* [#335](https://github.com/xmrig/xmrig-proxy/issues/335) Added support for unlimited algorithm switching.
* Config files from previous versions NOT compatible, `variant` option replaced to `algo`, global option `algo` removed.
* Command line options also not compatible, `--variant` option replaced to `--algo`.
* Algorithm `cn/msr` renamed to `cn/fast`.
* Algorithm `cn/xtl` removed.

# v2.16.1-beta
- Added RandomXL algorithm for [Loki](https://loki.network/).
  - Algorithm name used by proxy is `randomx/loki` or `rx/loki`.
  
# v2.16.0-beta
- [#1036](https://github.com/xmrig/xmrig/pull/1036) Added RandomWOW (RandomX with different preferences) algorithm support for [Wownero](http://wownero.org/).
  - Algorithm name used by proxy is `randomx/wow` or `rx/wow`.
  - Currently runtime algorithm switching NOT supported with other algorithms.
  
# v2.15.3-beta
- [#1014](https://github.com/xmrig/xmrig/issues/1014) Fixed regression, default value for `algo` option was not applied.

# v2.15.2-beta
- [#1010](https://github.com/xmrig/xmrig/pull/1010#issuecomment-482632107) Added daemon support (solo mining).
- Config subsystem was rewritten, internally JSON is primary format now.
- Fixed regression, big HTTP responses was truncated.

# v2.15.1-beta
- [#1007](https://github.com/xmrig/xmrig/issues/1007) Old HTTP API backend based on libmicrohttpd, replaced to custom HTTP server (libuv + http_parser).
- [#257](https://github.com/xmrig/xmrig-nvidia/pull/257) New logging subsystem, file and syslog now always without colors.

# v2.15.0-beta
- [#314](https://github.com/xmrig/xmrig-proxy/issues/314) Added donate over proxy feature and changed donation model.
  - Added new options `algo-ext` and `access-password`.
  - Added real graceful exit.

# v2.14.4
- Fixed MSVC 2019 version detection.
- Removed obsolete automatic variants.

# v2.14.1
- [#306](https://github.com/xmrig/xmrig-proxy/issues/306) [#310](https://github.com/xmrig/xmrig-proxy/issues/310) Fixed compile issues and random crashing if verbose mode or access log was enabled.

# v2.14.0
- **[#969](https://github.com/xmrig/xmrig/pull/969) Added new algorithm `cryptonight/rwz`, short alias `cn/rwz` (also known as CryptoNight ReverseWaltz), for upcoming [Graft](https://www.graft.network/) fork.**
- **[#931](https://github.com/xmrig/xmrig/issues/931) Added new algorithm `cryptonight/zls`, short alias `cn/zls` for [Zelerius Network](https://zelerius.org) fork.**
- **[#940](https://github.com/xmrig/xmrig/issues/940) Added new algorithm `cryptonight/double`, short alias `cn/double` (also known as CryptoNight HeavyX), for [X-CASH](https://x-cash.org/).**

# v2.13.0
- **[#938](https://github.com/xmrig/xmrig/issues/938) Added support for new algorithm `cryptonight/r`, short alias `cn/r` (also known as CryptoNightR or CryptoNight variant 4), for upcoming [Monero](https://www.getmonero.org/) fork on March 9, thanks [@SChernykh](https://github.com/SChernykh).**

# v2.12.0
- [#929](https://github.com/xmrig/xmrig/pull/929) Added support for new algorithm `cryptonight/wow`, short alias `cn/wow` (also known as CryptonightR), for upcoming [Wownero](http://wownero.org) fork on February 14.

# v2.11.0
- [#928](https://github.com/xmrig/xmrig/issues/928) Added support for new algorithm `cryptonight/gpu`, short alias `cn/gpu` (original name `cryptonight-gpu`), for upcoming [Ryo currency](https://ryo-currency.com) fork on February 14.

# v2.10.0
- [#904](https://github.com/xmrig/xmrig/issues/904) Added new algorithm `cn-pico/trtl` (aliases `cryptonight-turtle`, `cn-trtl`) for upcoming TurtleCoin (TRTL) fork.

# v2.9.4
- [#913](https://github.com/xmrig/xmrig/issues/913) Fixed Masari (MSR) support (this update required for upcoming fork).

# v2.9.1
- Restored compatibility with https://stellite.hashvault.pro.

# v2.9.0
- [#275](https://github.com/xmrig/xmrig-proxy/issues/275) Added SSL/TLS support for incoming connections.
- [#899](https://github.com/xmrig/xmrig/issues/899) Added support for new algorithm `cn/half` for Masari and Stellite forks.
- [#271](https://github.com/xmrig/xmrig-proxy/issues/271) Fixed broken pool options cascading (mixed configuration).
- Added memory and load_average information to API.

# v2.8.1
- [#258](https://github.com/xmrig/xmrig-proxy/issues/258) Force NDEBUG for release builds.
- [#108](https://github.com/xmrig/xmrig-proxy/issues/108) Fixed possible crash in simple mode when heavy load.
- [#777](https://github.com/xmrig/xmrig/issues/777) Better report about pool connection issues. 
- Fixed error when handle malformed result from miner (divide to zero).
- Fixed malformed login reply.

# v2.8.0
- **[#753](https://github.com/xmrig/xmrig/issues/753) Added new algorithm [CryptoNight variant 2](https://github.com/xmrig/xmrig/issues/753) for Monero fork, thanks [@SChernykh](https://github.com/SChernykh).**
- **[#251](https://github.com/xmrig/xmrig-proxy/issues/251) Added extended workers support.**
- **[#758](https://github.com/xmrig/xmrig/issues/758) Added SSL/TLS support for secure outgoing connections to pools.**
  - Added per pool options `"tls"` and `"tls-fingerprint"` and command line equivalents.
- [#757](https://github.com/xmrig/xmrig/issues/757) Fixed send buffer overflow.

# v2.6.5
- [#245](https://github.com/xmrig/xmrig-proxy/issues/245) Fixed API ID collision when run multiple proxies on same machine.
  - Added command line option `--api-id` and equivalent option for config file.
- Added `algo` field to API `GET /1/summary` endpoint.

# v2.6.4
- [#238](https://github.com/xmrig/xmrig-proxy/issues/238) `cryptonight-lite/ipbc` replaced to `cryptonight-heavy/tube`.
- Added `cryptonight/xao` and `cryptonight/rto` for future use.

# v2.6.3
- **Added support for new cryptonight-heavy variant xhv** (`cn-heavy/xhv`) for upcoming Haven Protocol fork.
- **Added support for new cryptonight variant msr** (`cn/msr`) also known as `cryptonight-fast` for upcoming Masari fork.
- Changed behavior for automatic variant to allow pool override algorithm.
- Fixed `--api-ipv6` option.
- [#629](https://github.com/xmrig/xmrig/pull/629) Fixed file logging with non-seekable files.
- [#672](https://github.com/xmrig/xmrig/pull/672) Reverted back `cryptonight-light` and exit if no valid algorithm specified.

# v2.6.2
 - [#197](https://github.com/xmrig/xmrig-proxy/issues/197) Fixed compatibility with xmr-stak `rig_id` option, xmr-stak sent empty rig id if user not specify it.
 - [#199](https://github.com/xmrig/xmrig-proxy/issues/199) Fixed various bugs in donation subsystem.

# v2.6.0
 - [#168](https://github.com/xmrig/xmrig-proxy/issues/168) Added support for [mining algorithm negotiation](https://github.com/xmrig/xmrig-proxy/blob/dev/doc/STRATUM_EXT.md#1-mining-algorithm-negotiation).
 - Added support for **rig-id** stratum protocol extensions, compatible with xmr-stak.
 - A lot of small fixes and better unification with miner code.

# v2.5.3
- Fixed critical bug, in some cases proxy was can't recovery connection and switch to failover pool, version 2.5.2 affected.
- Added configurable keepalive support, now possible override default timeout (60 seconds) via config file (only).
- Fixed wrong miners count in 32 bit builds.

# v2.5.2
- [#448](https://github.com/xmrig/xmrig/issues/478) Fixed broken reconnect.

# v2.5.0
- [#119](https://github.com/xmrig/xmrig-proxy/issues/119) Added graceful reload support, pools and some other settings now can changed without proxy restart.
- [#123](https://github.com/xmrig/xmrig-proxy/issues/123) Fixed regression (all versions since 2.4 affected) fragmented responses from pool/miner was parsed incorrectly.
- [#40](https://github.com/xmrig/xmrig-proxy/issues/40#issuecomment-370202169) Added API endpoint `PUT /1/config` to update current config.
- [#118](https://github.com/xmrig/xmrig-proxy/issues/118#issuecomment-375172833) Added alternative working mode, in that mode proxy support chaining and nicehash.com but lose ability to reduce connection count.
- Added API endpoint `GET /1/config` to get current active config.
- Messages `use pool` now shown only in verbose mode.
- Added IPv6 support:
  - IPv6 now fully supported for connections to upstream pools.
  - `bind` now accept IPv6 addresses, for example, use `[::]:3333` to bind on all IPv6 interfaces and port 3333. 
  - Internal HTTP server now support IPv6 for incoming connections.
- New command line options (with equivalent config file options):
  - Added `--mode` to switch working mode.
  - Added `--reuse-timeout` to set timeout in seconds for reuse pool connections in simple mode.
  - Added `--no-watch` and config option `watch` to disable config file watching.
  - Added `--variant` to override PoW settings on xmrig miners.
  - Added `--api-no-ipv6` and similar config option to disable IPv6 support for HTTP API.
  - Added `--algo` to specify algorithm cryptonight or cryptonight-lite.
  - Added `--api-no-restricted` to enable full access to api, this option has no effect if `--api-access-token` not specified.
- Deprecations:
  - Option `coin` now deprecated, use `algo` instead.
  - API endpoint `GET /` now deprecated, use `GET /1/summary` instead.
  - API endpoint `GET /workers.json`, use `GET /1/workers` instead.

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
