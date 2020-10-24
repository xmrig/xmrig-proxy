# v6.4.0
- [#1862](https://github.com/xmrig/xmrig/pull/1862) RandomX: removed `rx/loki` algorithm.
- [#1890](https://github.com/xmrig/xmrig/pull/1890) Added `argon2/chukwav2` algorithm.

# v6.3.0
- Sync changes with the miner.
  - Added support for upcoming Haven fork.
  - Added tags to log records.
  - [#1708](https://github.com/xmrig/xmrig/issues/1708) Added `title` option.
  - [#1728](https://github.com/xmrig/xmrig/issues/1728) Fixed, 32 bit Windows builds was crash on start.

# v5.11.0
- Added new algorithm `cn/ccx` for Conceal.
- Removed previously deprecated `cn/gpu` algorithm.

# v5.10.2
- [#1664](https://github.com/xmrig/xmrig/pull/1664) Improved JSON config error reporting.
- Fixed memory leak in HTTP client.
- Build [dependencies](https://github.com/xmrig/xmrig-deps/releases/tag/v4.1) updated to recent versions.

# v5.10.1
- [#1306](https://github.com/xmrig/xmrig/issues/1306) Fixed possible double connection to a pool.
- [#1654](https://github.com/xmrig/xmrig/issues/1654) Fixed build with LibreSSL.

# v5.10.0
- [#1596](https://github.com/xmrig/xmrig/issues/1596) Major TLS (Transport Layer Security) subsystem update.
- Reduced memory consumption by 4 kB per connection.
- Added command line option `--tls-gen`.
- Added command line option `--data-dir`.

# v5.9.0
- [#1578](https://github.com/xmrig/xmrig/pull/1578) Added new RandomKEVA algorithm for upcoming Kevacoin fork, as `"algo": "rx/keva"` or `"coin": "keva"`.

# v5.8.1
- [#1575](https://github.com/xmrig/xmrig/pull/1575) Fixed new block detection for DERO solo mining.

# v5.8.0
- [#1573](https://github.com/xmrig/xmrig/pull/1573) Added new AstroBWT algorithm for upcoming DERO fork, as `"algo": "astrobwt"` or `"coin": "dero"`.

# v5.7.0
- **Added SOCKS5 proxies support for Tor https://xmrig.com/docs/miner/tor.**
- [#377](https://github.com/xmrig/xmrig-proxy/issues/377) Fixed duplicate jobs in daemon (solo) mining client.
- Removed `libuuid` dependency on Linux.
- Fixed possible crashes in HTTP client.

# v5.5.1
- [#1469](https://github.com/xmrig/xmrig/issues/1469) Fixed build with gcc 4.8.
- Added environment variables support for TLS settings: `cert`, `cert_key`, `dhparam`.

# v5.5.0
- [#179](https://github.com/xmrig/xmrig/issues/179) Added support for [environment variables](https://xmrig.com/docs/miner/environment-variables) in config file.
- [#375](https://github.com/xmrig/xmrig-proxy/pull/375) Bugfixes: 64bit diff in logs + `"print-time"` config.
- [#376](https://github.com/xmrig/xmrig-proxy/pull/376) Added support for custom-diff shares for better proxy and worker stats.
  - Added `"custom-diff-stats"` config option.
  - Added `--custom-diff-stats` command line option.
- [#1445](https://github.com/xmrig/xmrig/pull/1445) Removed `rx/v` algorithm.
- [#1466](https://github.com/xmrig/xmrig/pull/1466) Added `cn-pico/tlo` algorithm.
- Added console title for Windows with proxy name and version.

# v5.4.0
- [#1434](https://github.com/xmrig/xmrig/pull/1434) Added RandomSFX (`rx/sfx`) algorithm for Safex Cash.
- [#1445](https://github.com/xmrig/xmrig/pull/1445) Added RandomV (`rx/v`) algorithm for *new* MoneroV.
- [#367](https://github.com/xmrig/xmrig-proxy/issues/367) Added "cert-key" alias, fixed --tls-cert-key command line option.

# v5.0.1
- **Fixed memory leak.**
- Fixed crash if no valid configuration found.
- Other minor fixes.

# v5.0.0
- Proxy rebased to latest miner codebase.
  - [#1068](https://github.com/xmrig/xmrig/pull/1068) Added support for `self-select` stratum protocol extension.
  - [#1227](https://github.com/xmrig/xmrig/pull/1227) Added new algorithm `rx/arq`, RandomX variant for upcoming ArQmA fork.

# v3.2.1
- [#349](https://github.com/xmrig/xmrig-proxy/issues/349) Fixed command line option `--coin`.

# v3.2.0
- Added per pool option `coin` with single possible value `monero` for pools without algorithm negotiation, for upcoming Monero fork.
- [#1183](https://github.com/xmrig/xmrig/issues/1183) Fixed compatibility with systemd.

# v3.1.1
- [#1133](https://github.com/xmrig/xmrig/issues/1133) Fixed syslog regression.
- [#1138](https://github.com/xmrig/xmrig/issues/1138) Fixed multiple network bugs.
- [#1141](https://github.com/xmrig/xmrig/issues/1141) Fixed log in background mode.
- Fixed command line options for single pool, free order allowed again.

# v3.1.0
- [#1107](https://github.com/xmrig/xmrig/issues/1107#issuecomment-522235892) Added Argon2 algorithm family: `argon2/chukwa` and `argon2/wrkz`.

# v3.0.0
- **[#1111](https://github.com/xmrig/xmrig/pull/1111) Added RandomX (`rx/test`) algorithm for testing and benchmarking.**
- **[#1036](https://github.com/xmrig/xmrig/pull/1036) Added RandomWOW (`rx/wow`) algorithm for [Wownero](http://wownero.org/).**
- **[#1050](https://github.com/xmrig/xmrig/pull/1050) Added RandomXL (`rx/loki`) algorithm for [Loki](https://loki.network/).**
- **[#335](https://github.com/xmrig/xmrig-proxy/issues/335) Added support for unlimited algorithm switching.**
- [#257](https://github.com/xmrig/xmrig-nvidia/pull/257) New logging subsystem, file and syslog now always without colors.
- [#314](https://github.com/xmrig/xmrig-proxy/issues/314) Added donate over proxy feature and changed donation model.
- [#1007](https://github.com/xmrig/xmrig/issues/1007) Old HTTP API backend based on libmicrohttpd, replaced to custom HTTP server (libuv + http_parser).
- [#1010](https://github.com/xmrig/xmrig/pull/1010#issuecomment-482632107) Added daemon support (solo mining).
- [#1066](https://github.com/xmrig/xmrig/issues/1066#issuecomment-518080529) Added error message if pool not ready for RandomX.
- Added new options `algo-ext` and `access-password`.
- Config files from previous versions NOT compatible, `variant` option replaced to `algo`, global option `algo` removed.
- Command line options also not compatible, `--variant` option replaced to `--algo`.
- Algorithm `cn/msr` renamed to `cn/fast`.
- Algorithm `cn/xtl` removed.

# Previous versions
[doc/CHANGELOG_OLD.md](doc/CHANGELOG_OLD.md)
