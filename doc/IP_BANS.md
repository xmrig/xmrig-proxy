# Pool-triggered IP bans

The proxy bans a miner connection source IP for 24 hours by default when an
upstream pool rejects a forwarded share with the exact error `Low difficulty share`, compared
case-insensitively using ASCII. Set `--ip-ban-hours=N` on the command line or
`"ip-ban-hours": N` at the top level of the JSON config to change the duration.
`N` must be a whole number from 0 to 4,294,967,295. Setting it to `0` disables
the ban: the proxy still logs the bad share, its source IP, and that it will be
banned for `0h`, but keeps miners connected and permits reconnections.

The first rejection starts the fixed monotonic window; later matching
rejections do not extend it. The ban uses the connection's source IP, so all
currently connected miners behind the same NAT
are closed and later connections from that IP are closed after registration.

The in-memory cache is cleared on restart or when the configured duration
changes, including a live config reload, and holds at most 65,536 live IPs.
When full, its oldest live entry is evicted. Expired entries are pruned in FIFO
order. Only upstream rejection creates a ban. Other pool errors remain excluded;
local share checks and local lower-difficulty acceptances do not create bans,
while forwarded shares remain covered when a miner uses custom difficulty. The
proxy does not add local hash validation.
