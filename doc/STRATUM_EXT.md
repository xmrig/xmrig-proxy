# Stratum protocol extensions
## 1. Mining algorithm negotiation
Subset of protocol extensions, used to negotiate algorithm between miner and pool/proxy. All extensions is backward compatible with standart stratum protocol.

### 1.1. Miner defined algorithms list
Miner should send list of [algorithms](#14-algorithm-names-and-variants) supported. Multiple algorithms in list meant miner can switch algorithms in runtime.
```json
{
  "id": 1, "jsonrpc": "2.0", "method": "login",
  "params": {
    "login": "...", "pass": "...", "agent": "...",
    "algo": ["cn", "cn/0", "cn/1", "cn/xtl"]
  }
}
```
In case if miner not support dynamic algorithm change, miner should send list with one item, for example `"algo": ["cn-heavy"]`, pool/proxy should provide work for selected algorithm or send error.

### 1.2. Extended job object
To each `job` object pool/proxy should add additional field `algo`.

```json
{
  "id": 1, "jsonrpc": "2.0", "error": null,
  "result": {
    "id": "...",
    "job": {
      "blob": "...", "job_id": "...", "target": "...", "id": "...",
      "algo": "cn/r"
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
    "algo": "cn/r"
  }
}
```

If miner not support algorithm connection should be closed by miner to initiate switch to backup pool.

### 1.3. Algo extension
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

Second, miner add fields `algo` to submit request.
```json
{
  "id": 2, "jsonrpc": "2.0", "method": "submit",
  "params": {
    "id": "...", "job_id": "...", "nonce": "...", "result": "...",
    "algo": "cn/1"
  }
}
```

### 1.4 Algorithm names and variants
* https://github.com/xmrig/xmrig/blob/master/doc/ALGORITHMS.md#algorithm-names

## Rig identifier
User defined rig identifier. Optional field `rigid` in `login` request. More details: https://github.com/fireice-uk/xmr-stak/issues/849

## Extended mining statistics
More details: https://github.com/fireice-uk/xmr-stak/issues/66
