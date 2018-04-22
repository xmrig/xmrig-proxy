# Stratum protocol extensions
## Mining algorithm negotiation
Subset of protocol extensions, used to negotiate algorithm between miner and pool/proxy. All extensions is backward compatible with standart stratum protocol.

### Miner defined algorithms list
Miner should send list of algorithms supported. Multiple algorithms in list meant miner can switch algorithms in runtime.
```json
{
  "id": 1, "jsonrpc": "2.0", "method": "login",
  "params": {
    "login": "...", "pass": "...", "agent": "...",
    "algo": ["cn", "cn-lite", "cn-heavy"]
  }
}
```
In case if miner not support dynamic algorithm change, miner should send list with one item, for example `"algo": ["cn-heavy"]`, pool/proxy should provide work for selected algorithm or send error.

### Extended job object
To each `job` object pool/proxy should add 2 additional fields `algo` and `variant`.

```json
{
  "id": 1, "jsonrpc": "2.0", "error": null,
  "result": {
    "id": "...",
    "job": {
      "blob": "...", "job_id": "...", "target": "...", "id": "...",
      "algo": "cn", "variant": 1
    },
    "status": "OK"
  }
}
```

```json
{
  "jsonrpc": "2.0", "method": "job",
  "params": {
    "blob": "...", "job_id": "...", "target": "b88d0600", "id": "...",
    "algo": "cn", "variant": 1
  }
}
```
Possible values for `variant`:

* `1` Force use variant 1 of algorithm.
* `0` Force use original cn/cn-lite algorithm.
* `-1` or missing field, leave miner autodetect algorithm by block version.

Note about `cn-heavy` this algorithm now support only one (original) variant, so only valid values `-1` or `0`. `1` is reserved for future use, current pool/proxy implementation should never send `"variant": 1` if `cn-heavy` algorithm used.

If miner not support algorithm connection should be closed by miner to initiate switch to backup pool.

### Algo extension
This extension is backward compatible with xmr-stak [extended mining statistics](#extended-mining-statistics).
First, pool should add `algo` to extensions list:
```json
{
  "id": 1, "jsonrpc": "2.0", "error": null,
  "result": {
    "id": "...",
    "job": {
      "blob": "...", "job_id": "...", "target": "...", "id": "...",
      "algo": "cn", "variant": 1
    },
    "extensions" : ["algo"],
    "status": "OK"
  }
}
```

Second, miner add fields `algo` and `variant` to submit request.
```json
{
  "id": 2, "jsonrpc": "2.0", "method": "submit",
  "params": {
    "id": "...", "job_id": "...", "nonce": "...", "result": "...",
    "algo": "cn", "variant": 1
  }
}
```

Note about xmr-stak, this miner not send `variant` field and always use long algorithm names, also used 2 non standart algorithm names `cryptonight-monerov7` and `cryptonight-aeonv7`, pool side should support it as `cn` variant 1 and `cn-lite` variant 1.

### Algorithm names and variants
Both miner and pool should support short algorithm name aliases:

| Long name         | Short name | Variants   |
|-------------------|------------|------------|
| cryptonight       | cn         | `0` or `1` |
| cryptonight-lite  | cn-lite    | `0` or `1` |
| cryptonight-heavy | cn-heavy   | only `0`   |
| cryptonight-ipbc  | cn-ipbc    | only `1`   |

Note about **cryptonight** and **cryptonight variant 1**, also known as **cryptonight v7**, all these variants use same algorithm name `cryptonight` or `cn`, miner should able to switch between variants in runtime.

## Rig identifier
User defined rig identifier. Optional field `rigid` in `login` request. More details: https://github.com/fireice-uk/xmr-stak/issues/849

## Extended mining statistics
More details: https://github.com/fireice-uk/xmr-stak/issues/66
