# Stratum protocol extensions
## 1. Mining algorithm negotiation
Subset of protocol extensions, used to negotiate algorithm between miner and pool/proxy. All extensions is backward compatible with standart stratum protocol.

### 1.1. Miner defined algorithms list
Miner should send list of algorithms supported. Multiple algorithms in list meant miner can switch algorithms in runtime.
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
To each `job` object pool/proxy should add additional field `algo` and optional `variant` field.

```json
{
  "id": 1, "jsonrpc": "2.0", "error": null,
  "result": {
    "id": "...",
    "job": {
      "blob": "...", "job_id": "...", "target": "...", "id": "...",
      "algo": "cn/1", "variant": 1
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
    "algo": "cn/1"
  }
}
```
Possible values for `variant`:

* `1` Force use variant 1 of algorithm.
* `0` Force use original cn/cn-lite algorithm.

This field used for backward compatibility with xmrig 2.5, new miner implementations should support only `algo`.

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

Note about xmr-stak, this miner use different algrotim names, comparsion table will be added later.

### 1.4 Algorithm names and variants
Both miner and pool should support short algorithm name aliases:

| Long name               | Short name     | Base algorithm      | Variant     | Notes                                                |
|-------------------------|----------------|---------------------|-------------|------------------------------------------------------|
| `cryptonight`           | `cn`           | `cryptonight`       | `-1`        | Autodetect works only for Monero.                    |
| `cryptonight/0`         | `cn/0`         | `cryptonight`       | `0`         | Original/old CryptoNight.                            |
| `cryptonight/1`         | `cn/1`         | `cryptonight`       | `1`         | Also known as `monero7` and `CryptoNightV7`.         |
| `cryptonight/xtl`       | `cn/xtl`       | `cryptonight        | `xtl`       | Stellite (XTL) variant.                              |
| `cryptonight-lite`      | `cn-lite`      | `cryptonight-lite`  | `-1`        | Autodetect works only for Aeon.                      |
| `cryptonight-lite/0`    | `cn-lite/0`    | `cryptonight-lite`  | `0`         | Original/old CryptoNight-Lite.                       |
| `cryptonight-lite/1`    | `cn-lite/1`    | `cryptonight-lite`  | `1`         | Also known as `aeon7`                                |
| `cryptonight-lite/ipbc` | `cn-lite/ipbc` | `cryptonight-lite`  | `ipbc`      | IPBC variant                                         |
| `cryptonight-heavy`     | `cn-heavy`     | `cryptonight-heavy` | `0`         | Sumokoin and Haven Protocol                          |

## Rig identifier
User defined rig identifier. Optional field `rigid` in `login` request. More details: https://github.com/fireice-uk/xmr-stak/issues/849

## Extended mining statistics
More details: https://github.com/fireice-uk/xmr-stak/issues/66
