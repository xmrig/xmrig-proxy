"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const net = require("node:net");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");

const { FakeMiner, FakePool, delay, waitFor, withProxy } = require("./common/proxy_harness.js");
const { NativePool, target } = require("./common/native_fixtures.js");

const LOW_DIFFICULTY = "Low difficulty share";

async function canBindDistinctLoopback() {
    const server = net.createServer();

    try {
        await new Promise((resolve, reject) => {
            const onError = error => {
                server.off("listening", onListening);
                reject(error);
            };
            const onListening = () => {
                server.off("error", onError);
                resolve();
            };

            server.once("error", onError);
            server.once("listening", onListening);
            server.listen(0, "127.0.0.2");
        });
        return true;
    }
    catch (error) {
        return false;
    }
    finally {
        if (server.listening) {
            await new Promise(resolve => server.close(resolve));
        }
    }
}

async function requireDistinctLoopback(t) {
    if (await canBindDistinctLoopback()) {
        return true;
    }

    t.skip("127.0.0.2 is not bindable on this host; distinct source-IP coverage skipped");
    return false;
}

function hashAtDifficulty(difficulty) {
    const tail = Buffer.alloc(8);
    tail.writeBigUInt64LE(((1n << 64n) - 1n) / BigInt(difficulty));
    return "00".repeat(24) + tail.toString("hex");
}

function legacyCapabilities() {
    return { algos: ["cn-heavy/xhv"] };
}

function sendLegacyShare(miner, id, difficulty, overrides = {}) {
    const login = miner.peer.messages.find(message => message.id === 1);
    assert.ok(login && login.result && login.result.id, `${miner.name} has no object login response`);
    const job = miner.lastJob;
    assert.ok(job && job.job_id, `${miner.name} has no object job`);

    miner.peer.send({ id, method: "submit", params: Object.assign({
        id: login.result.id,
        job_id: job.job_id,
        nonce: job.blob.slice(78, 86),
        result: hashAtDifficulty(difficulty),
        algo: job.algo
    }, overrides) });
}

async function waitClosed(miner, description) {
    await waitFor(() => miner.peer && miner.peer.closed, miner.timeoutMs, description);
}

async function connectOnly(miners, name, proxyPort, timeoutMs, localAddress) {
    const miner = new FakeMiner(name, proxyPort, timeoutMs, { localAddress });
    miners.push(miner);
    await miner.connect();
    return miner;
}

class DelayedSharePool extends FakePool {
    constructor(timeoutMs, options = {}) {
        super(timeoutMs, options);
        this.pendingSubmits = [];
    }

    onMessage(connection, message) {
        if (message.method === "submit") {
            const pending = { connection, message };
            this.submits.push(pending);
            this.pendingSubmits.push(pending);
            return;
        }

        return super.onMessage(connection, message);
    }

    replySubmit(index, kind = "accepted") {
        const pending = this.pendingSubmits[index];
        assert.ok(pending, `missing held object submit ${index}`);
        const response = { id: pending.message.id, jsonrpc: "2.0", error: null, result: { status: "OK" } };

        if (kind === "false") {
            response.result = false;
        }
        else if (kind !== "accepted") {
            response.error = { code: -1, message: kind };
            response.result = null;
        }

        pending.connection.peer.send(response);
    }
}

class DelayedNativePool extends NativePool {
    constructor(timeoutMs) {
        super(timeoutMs);
        this.pendingNativeSubmits = [];
    }

    onMessage(connection, message) {
        if (message.method === "mining.submit") {
            const pending = { connection, message };
            this.submits.push(pending);
            this.pendingNativeSubmits.push(pending);
            return;
        }

        return super.onMessage(connection, message);
    }

    replyNativeSubmit(index, kind = "accepted") {
        const pending = this.pendingNativeSubmits[index];
        assert.ok(pending, `missing held native submit ${index}`);
        const response = { id: pending.message.id, jsonrpc: "2.0", error: null, result: true };

        if (kind !== "accepted") {
            response.error = { code: -1, message: kind };
            response.result = null;
        }

        pending.connection.peer.send(response);
    }
}

async function waitForNativeJob(pool, miner, id) {
    await pool.waitForGetjobs(1);
    pool.push(pool.connections[0], "kawpow", id);
    return miner.peer.waitForMessage(
        message => message.method === "mining.notify" && message.params && message.params[0] === id,
        miner.timeoutMs, `native job ${id}`);
}

async function sendNativeShare(miner, id, jobId) {
    const subscriptionId = `${id}-subscribe`;
    miner.peer.send({ id: subscriptionId, method: "mining.subscribe", params: ["offline-ip-ban"] });
    const subscription = await miner.peer.waitForMessage(message => message.id === subscriptionId,
        miner.timeoutMs, `${miner.name} native subscription`);
    assert.equal(subscription.error, null);
    assert.ok(subscription.result && subscription.result[1], `${miner.name} has no native extra nonce`);
    const nonce = `0x${subscription.result[1].padEnd(16, "0")}`;
    miner.peer.send({ id, method: "mining.submit", params: [
        miner.name, jobId, nonce, `0x${"12".repeat(32)}`, `0x${"56".repeat(32)}`
    ], result: target(20000) });
}

test.describe("proxy IP bans", { concurrency: false }, () => {
    test("FakeMiner.login retains a newer job notification delivered with the login response", async () => {
        const loginJob = { job_id: "login-job-a", height: 2001 };
        const notificationJob = { job_id: "notification-job-b", height: 2002 };
        const server = net.createServer(socket => {
            let request = "";
            socket.on("error", () => {});
            socket.on("data", chunk => {
                request += chunk.toString();
                if (!request.includes("\n")) {
                    return;
                }

                socket.end(`${JSON.stringify({
                    id: 1,
                    jsonrpc: "2.0",
                    error: null,
                    result: { id: "pool-race", job: loginJob }
                })}\n${JSON.stringify({
                    jsonrpc: "2.0",
                    method: "job",
                    params: notificationJob
                })}\n`);
                socket.removeAllListeners("data");
            });
        });

        await new Promise((resolve, reject) => {
            server.once("error", reject);
            server.listen(0, "127.0.0.1", resolve);
        });

        const miner = new FakeMiner("login-race", server.address().port, 1000);
        try {
            await miner.connect();
            await miner.login();
            assert.equal(miner.lastJob.job_id, "notification-job-b");
        }
        finally {
            miner.close();
            await new Promise(resolve => server.close(resolve));
        }
    });

    test("ordinary low-difficulty rejection bans the source IP and preserves an accepted other IP", async t => {
        if (!await requireDistinctLoopback(t)) {
            return;
        }

        await withProxy(async ({ addMiner, miners, pool, config, proxy, proxyPort }) => {
            const sameA = await addMiner("ban-simple-a", legacyCapabilities(), { localAddress: "127.0.0.1" });
            const sameB = await addMiner("ban-simple-b", legacyCapabilities(), { localAddress: "127.0.0.1" });
            const other = await addMiner("ban-simple-other", legacyCapabilities(), { localAddress: "127.0.0.2" });

            sendLegacyShare(sameA, 20, 20000);
            await pool.waitForSubmits(1);
            pool.replySubmit(0, LOW_DIFFICULTY);

            await Promise.all([
                waitClosed(sameA, "low-difficulty miner close"),
                waitClosed(sameB, "same-IP miner close")
            ]);
            await waitFor(() => proxy.output.join("").includes('banned IP "127.0.0.1" for 24h after low difficulty share'),
                config.timeoutMs, "default IP ban warning");
            assert.equal(other.peer.closed, false, "different source IP was disconnected");

            sendLegacyShare(other, 21, 20000);
            await pool.waitForSubmits(2);
            pool.replySubmit(1, "accepted");
            const accepted = await other.peer.waitForMessage(message => message.id === 21,
                config.timeoutMs, "accepted share response");
            assert.equal(accepted.error, null);

            const reconnect = await connectOnly(miners, "ban-simple-reconnect", proxyPort,
                config.timeoutMs, "127.0.0.1");
            await waitClosed(reconnect, "banned source reconnect close");
        }, {
            poolFactory: timeout => new DelayedSharePool(timeout),
            proxyArgs: ["--mode=simple"],
            waitForInitialPoolLogin: false
        });
    });

    test("custom CLI duration bans the source IP and reports the configured hours", async () => {
        await withProxy(async ({ addMiner, miners, pool, config, proxy, proxyPort }) => {
            const miner = await addMiner("ban-custom", legacyCapabilities(), { localAddress: "127.0.0.1" });
            sendLegacyShare(miner, 22, 20000);
            await pool.waitForSubmits(1);
            pool.replySubmit(0, "LOW DIFFICULTY SHARE");
            await waitClosed(miner, "custom-duration miner close");
            await waitFor(() => proxy.output.join("").includes('banned IP "127.0.0.1" for 7h after low difficulty share'),
                config.timeoutMs, "custom IP ban warning");

            const reconnect = await connectOnly(miners, "ban-custom-reconnect", proxyPort,
                config.timeoutMs, "127.0.0.1");
            await waitClosed(reconnect, "custom-duration source reconnect close");
        }, {
            poolFactory: timeout => new DelayedSharePool(timeout),
            proxyArgs: ["--ip-ban-hours=7"]
        });
    });

    for (const source of ["CLI", "JSON"]) {
        test(`${source} zero duration warns without disconnecting miners or blocking reconnect`, async () => {
            const dir = fs.mkdtempSync(path.join(os.tmpdir(), "proxy-ip-ban-config-"));
            const configPath = path.join(dir, "config.json");
            fs.writeFileSync(configPath, JSON.stringify({ "ip-ban-hours": 0, watch: false }));
            const proxyArgs = source === "CLI" ? ["--ip-ban-hours=0"] : ["--config", configPath];

            try {
                await withProxy(async ({ addMiner, miners, pool, config, proxy, proxyPort }) => {
                    const miner = await addMiner(`ban-disabled-${source}-a`, legacyCapabilities(), { localAddress: "127.0.0.1" });
                    const sameIp = await addMiner(`ban-disabled-${source}-b`, legacyCapabilities(), { localAddress: "127.0.0.1" });
                    const job = pool.broadcastJob()[0];
                    await Promise.all([
                        miner.waitForJob(value => value.job_id === job.job_id),
                        sameIp.waitForJob(value => value.job_id === job.job_id)
                    ]);
                    sendLegacyShare(miner, 23, 20000);
                    await pool.waitForSubmits(1);
                    pool.replySubmit(0, LOW_DIFFICULTY);
                    const rejected = await miner.peer.waitForMessage(message => message.id === 23,
                        config.timeoutMs, "ban-disabled rejected share response");
                    assert.equal(rejected.error.message, LOW_DIFFICULTY);
                    await waitFor(() => proxy.output.join("").includes('bad share from IP "127.0.0.1": low difficulty share; will be banned for 0h (IP ban disabled)'),
                        config.timeoutMs, "disabled IP ban warning");

                    for (const [index, active] of [miner, sameIp].entries()) {
                        assert.equal(active.peer.closed, false, "zero ban duration disconnected a same-IP miner");
                        sendLegacyShare(active, 24 + index, 20000);
                        await pool.waitForSubmits(2 + index);
                        pool.replySubmit(1 + index, "accepted");
                        const accepted = await active.peer.waitForMessage(message => message.id === 24 + index,
                            config.timeoutMs, "accepted share with IP bans disabled");
                        assert.equal(accepted.error, null);
                    }

                    const reconnect = await connectOnly(miners, `ban-disabled-${source}-reconnect`, proxyPort,
                        config.timeoutMs, "127.0.0.1");
                    await reconnect.login(legacyCapabilities());
                    assert.equal(reconnect.peer.closed, false, "zero ban duration blocked reconnect");
                }, { poolFactory: timeout => new DelayedSharePool(timeout), proxyArgs });
            }
            finally {
                fs.rmSync(dir, { recursive: true, force: true });
            }
        });
    }

    test("reversed pool replies are attributed to the submitting source IP", async t => {
        if (!await requireDistinctLoopback(t)) {
            return;
        }

        await withProxy(async ({ addMiner, miners, pool, config, proxyPort }) => {
            const first = await addMiner("ban-reverse-first", legacyCapabilities(), { localAddress: "127.0.0.1" });
            sendLegacyShare(first, 30, 20000);
            await pool.waitForSubmits(1);

            // Wait for the first upstream submit before the second membership refresh.
            const second = await addMiner("ban-reverse-second", legacyCapabilities(), { localAddress: "127.0.0.2" });
            sendLegacyShare(second, 31, 20000);
            await pool.waitForSubmits(2);

            pool.replySubmit(1, LOW_DIFFICULTY);
            await waitClosed(second, "reversed low-difficulty source close");
            assert.equal(first.peer.closed, false, "accepted source was disconnected by reversed reply");

            pool.replySubmit(0, "accepted");
            const accepted = await first.peer.waitForMessage(message => message.id === 30,
                config.timeoutMs, "reversed accepted share response");
            assert.equal(accepted.error, null);

            const reconnect = await connectOnly(miners, "ban-reverse-second-reconnect", proxyPort,
                config.timeoutMs, "127.0.0.2");
            await waitClosed(reconnect, "reversed banned source reconnect close");
        }, { poolFactory: timeout => new DelayedSharePool(timeout) });
    });

    test("a delayed rejection keeps the original source after the miner disconnects and mapper reuse", async t => {
        if (!await requireDistinctLoopback(t)) {
            return;
        }

        for (const mode of ["nicehash", "simple"]) {
            const proxyOptions = mode === "simple"
                ? {
                    poolFactory: timeout => new DelayedSharePool(timeout),
                    proxyArgs: ["--mode=simple", "--reuse-timeout=5"],
                    waitForInitialPoolLogin: false
                }
                : { poolFactory: timeout => new DelayedSharePool(timeout) };

            await withProxy(async ({ addMiner, miners, pool, config, proxyPort }) => {
                const original = await addMiner(`ban-delayed-${mode}-original`, legacyCapabilities(), { localAddress: "127.0.0.1" });
                sendLegacyShare(original, 40, 20000);
                await pool.waitForSubmits(1);
                original.close();
                await waitClosed(original, `${mode} original delayed miner close`);
                // The peer sees the socket close before the proxy's queued CloseEvent
                // necessarily removes the miner from its mapper.
                await delay(100);
                if (mode === "simple") {
                    // SimpleMapper marks a detached mapper dirty until its upstream
                    // publishes another job; refresh it before exercising reuse.
                    pool.broadcastJob();
                    await delay(50);
                }

                let replacement;
                if (mode === "simple") {
                    replacement = await connectOnly(miners, `ban-delayed-${mode}-replacement`, proxyPort,
                        config.timeoutMs, "127.0.0.2");
                    const login = replacement.login(legacyCapabilities());
                    await delay(50);
                    pool.broadcastJob();
                    await login;
                }
                else {
                    replacement = await addMiner(`ban-delayed-${mode}-replacement`, legacyCapabilities(), { localAddress: "127.0.0.2" });
                }
                assert.equal(pool.logins.length, 1, `${mode} replacement did not reuse the existing upstream group`);
                pool.replySubmit(0, LOW_DIFFICULTY);
                await delay(100);
                assert.equal(replacement.peer.closed, false, `${mode} replacement source was incorrectly banned`);

                const reconnect = await connectOnly(miners, `ban-delayed-${mode}-original-reconnect`, proxyPort,
                    config.timeoutMs, "127.0.0.1");
                await waitClosed(reconnect, `${mode} original delayed source reconnect close`);
            }, proxyOptions);
        }
    });

    test("stale, duplicate, and other pool errors do not ban, and local low difficulty stays local", async () => {
        for (const [index, error] of [
            [0, "Job not found"],
            [1, "Duplicate share"],
            [2, "pool rejected share"]
        ]) {
            await withProxy(async ({ addMiner, miners, pool, config, proxyPort }) => {
                const address = "127.0.0.1";
                const miner = await addMiner(`nonban-pool-error-${index}`, legacyCapabilities(), { localAddress: address });
                sendLegacyShare(miner, 50 + index, 20000);
                await pool.waitForSubmits(1);
                pool.replySubmit(0, error);
                const rejected = await miner.peer.waitForMessage(message => message.id === 50 + index,
                    config.timeoutMs, `${error} response`);
                assert.ok(rejected.error, `${error} did not reach miner`);
                assert.equal(miner.peer.closed, false, `${error} disconnected miner`);

                const reconnect = await connectOnly(miners, `nonban-pool-error-${index}-reconnect`,
                    proxyPort, config.timeoutMs, address);
                await reconnect.login(legacyCapabilities());
                assert.equal(reconnect.peer.closed, false, `${error} blocked reconnect`);
            }, { poolFactory: timeout => new DelayedSharePool(timeout) });
        }

        await withProxy(async ({ addMiner, miners, config, proxyPort, pool }) => {
            const address = "127.0.0.1";
            const miner = await addMiner("nonban-local-lowdiff+1000", legacyCapabilities(), { localAddress: address });
            sendLegacyShare(miner, 60, 100);
            const rejected = await miner.peer.waitForMessage(message => message.id === 60,
                config.timeoutMs, "locally rejected low-difficulty share");
            assert.ok(rejected.error);
            await delay(100);
            assert.equal(pool.submits.length, 0, "locally rejected share reached the pool");
            assert.equal(miner.peer.closed, false, "local low-difficulty rejection disconnected miner");

            const reconnect = await connectOnly(miners, "nonban-local-lowdiff-reconnect",
                proxyPort, config.timeoutMs, address);
            await reconnect.login(legacyCapabilities());
            assert.equal(reconnect.peer.closed, false, "local low-difficulty rejection blocked reconnect");
        }, { poolFactory: timeout => new DelayedSharePool(timeout) });
    });

    test("native low-difficulty rejection bans the native miner source IP", async () => {
        await withProxy(async ({ addMiner, miners, pool, config, proxyPort }) => {
            const miner = await addMiner("ban-native", {
                algos: ["cn-heavy/xhv", "kawpow"],
                perfs: { kawpow: 100 },
                params: { extensions: ["mo-native", "submit-result"] }
            }, { localAddress: "127.0.0.1" });
            const jobId = "ban-native-job";
            await waitForNativeJob(pool, miner, jobId);
            await sendNativeShare(miner, "native-low", jobId);
            await pool.waitForSubmits(1);
            pool.replyNativeSubmit(0, LOW_DIFFICULTY);
            await waitClosed(miner, "native low-difficulty miner close");

            const reconnect = await connectOnly(miners, "ban-native-reconnect", proxyPort,
                config.timeoutMs, "127.0.0.1");
            await waitClosed(reconnect, "native banned source reconnect close");
        }, { poolFactory: timeout => new DelayedNativePool(timeout) });
    });
});
