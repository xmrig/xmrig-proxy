# CONNECT protocol extension.

This stratum protocol extension allow miner use proxy for forwarding donation if direct connections to donation servers not possible.

## Extensions list
If proxy support this extension value `connect` should be added to extension list in login request reply and optionaly `tls` if proxy support SSL/TLS connections.

```json
{
  "id": 1, "jsonrpc": "2.0", "error": null,
  "result": {
    "id": "...",
    "job": { "blob": "...", "job_id": "...", "target": "...", "id": "...", "algo": "..." },
    "extensions" : ["algo","nicehash","connect","tls","keepalive"],
    "status": "OK"
  }
}
```

## Extended login request.

If miner detect current connection support this extension and donation time is come, miner will create new connection to proxy with one extra field `url` in login request.

```json
{
  "id": 1, "jsonrpc": "2.0", "method": "login",
  "params": {
    "login": "...", "pass": "...", "agent": "...", "algo": [...],
    "url": "stratum+ssl://donate.ssl.xmrig.com:443"
  }
}
```

Proxy will perform new connection to requested url, fields `user`, `rigid`, `agent`, `algo` will be forwarded as is to upstream donation server.
 
