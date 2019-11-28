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
