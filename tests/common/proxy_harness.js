"use strict";

const assert = require("node:assert/strict");
const { spawn, spawnSync } = require("node:child_process");
const { EventEmitter } = require("node:events");
const fs = require("node:fs");
const net = require("node:net");
const os = require("node:os");
const path = require("node:path");

const ROOT = path.resolve(__dirname, "..", "..");
const DEFAULT_TIMEOUT_MS = 15000;
const BASE_BLOB = "070780e6b9d60586ba419a0c224e3c6c3e134cc45c4fa04d8ee2d91c2595463c57eef0a4f0796c000000002fcc4d62fa6c77e76c30017c768be5c61d83ec9d3a085d524ba8053ecc3224660d";

const DEFAULT_ALGOS = ["rx/0", "rx/2", "cn-heavy/xhv", "cn/half"];
const DEFAULT_PERFS = {
    "rx/0": 1000,
    "rx/2": 1000,
    "cn-heavy/xhv": 10,
    "cn/half": 1
};

const CAPABILITIES = {
    base: {
        algos: ["rx/0", "cn-heavy/xhv"],
        perfs: {
            "rx/0": 1000,
            "cn-heavy/xhv": 10
        }
    },
    superset: {
        algos: ["rx/0", "rx/2", "cn-heavy/xhv", "cn/half"],
        perfs: {
            "rx/0": 1000,
            "rx/2": 950,
            "cn-heavy/xhv": 10,
            "cn/half": 1
        }
    }
};

function getTestConfig() {
    const binary = process.env.XMRIG_PROXY_TEST_BINARY || path.join(ROOT, "build", process.platform === "win32" ? "xmrig-proxy.exe" : "xmrig-proxy");
    const timeoutMs = Number(process.env.XMRIG_PROXY_TEST_TIMEOUT_MS || DEFAULT_TIMEOUT_MS);

    assert.ok(Number.isFinite(timeoutMs) && timeoutMs > 0, "XMRIG_PROXY_TEST_TIMEOUT_MS must be a positive number");

    return {
        binary,
        timeoutMs,
        verbose: process.env.XMRIG_PROXY_TEST_VERBOSE === "1"
    };
}

function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function waitFor(predicate, timeoutMs, description) {
    const started = Date.now();

    while (Date.now() - started < timeoutMs) {
        if (predicate()) {
            return;
        }

        await delay(25);
    }

    throw new Error(`timed out waiting for ${description}`);
}

async function getFreePort() {
    const server = net.createServer();

    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", resolve);
    });

    const port = server.address().port;
    await new Promise(resolve => server.close(resolve));

    return port;
}

class JsonPeer extends EventEmitter {
    constructor(socket, name) {
        super();
        this.socket = socket;
        this.name = name;
        this.buffer = "";
        this.messages = [];
        this.closed = false;

        socket.setEncoding("utf8");
        socket.on("data", data => this.onData(data));
        socket.on("error", error => this.emit("peer-error", error));
        socket.on("close", () => {
            this.closed = true;
            this.emit("closed");
        });
    }

    onData(data) {
        this.buffer += data;

        while (true) {
            const index = this.buffer.indexOf("\n");
            if (index < 0) {
                return;
            }

            const line = this.buffer.slice(0, index).trim();
            this.buffer = this.buffer.slice(index + 1);
            if (!line) {
                continue;
            }

            let message;
            try {
                message = JSON.parse(line);
            }
            catch (error) {
                error.message = `${this.name}: failed to parse JSON line ${line}: ${error.message}`;
                this.emit("peer-error", error);
                continue;
            }

            this.messages.push(message);
            this.emit("message", message);
        }
    }

    send(message) {
        this.socket.write(`${JSON.stringify(message)}\n`);
    }

    waitForMessage(predicate, timeoutMs, description) {
        const found = this.messages.find(predicate);
        if (found) {
            return Promise.resolve(found);
        }

        return new Promise((resolve, reject) => {
            const onMessage = message => {
                if (!predicate(message)) {
                    return;
                }

                cleanup();
                resolve(message);
            };
            const onClosed = () => {
                cleanup();
                reject(new Error(`${this.name} closed while waiting for ${description}`));
            };
            const onError = error => {
                cleanup();
                reject(error);
            };
            const timer = setTimeout(() => {
                cleanup();
                reject(new Error(`timed out waiting for ${description}`));
            }, timeoutMs);
            const cleanup = () => {
                clearTimeout(timer);
                this.off("message", onMessage);
                this.off("closed", onClosed);
                this.off("peer-error", onError);
            };

            this.on("message", onMessage);
            this.once("closed", onClosed);
            this.once("peer-error", onError);
        });
    }

    close() {
        this.socket.destroy();
    }
}

class FakePool {
    constructor(timeoutMs) {
        this.timeoutMs = timeoutMs;
        this.server = net.createServer(socket => this.onConnection(socket));
        this.connections = [];
        this.logins = [];
        this.getjobs = [];
        this.submits = [];
        this.jobSeq = 0;
    }

    async start() {
        await new Promise((resolve, reject) => {
            this.server.once("error", reject);
            this.server.listen(0, "127.0.0.1", resolve);
        });

        this.port = this.server.address().port;
    }

    onConnection(socket) {
        const connection = {
            id: this.connections.length + 1,
            rpcId: `pool-${this.connections.length + 1}`,
            peer: new JsonPeer(socket, `pool upstream ${this.connections.length + 1}`)
        };

        this.connections.push(connection);
        connection.peer.on("message", message => this.onMessage(connection, message));
    }

    onMessage(connection, message) {
        if (message.method === "login") {
            this.logins.push({ connection, message });
            connection.peer.send({
                id: message.id,
                jsonrpc: "2.0",
                error: null,
                result: {
                    id: connection.rpcId,
                    job: this.nextJob(),
                    extensions: ["algo", "keepalive"]
                }
            });
            return;
        }

        if (message.method === "getjob") {
            this.getjobs.push({ connection, message });
            connection.peer.send({
                id: message.id,
                jsonrpc: "2.0",
                error: null,
                result: Object.assign({
                    id: connection.rpcId,
                    extensions: ["algo", "keepalive"]
                }, this.nextJob())
            });
            return;
        }

        if (message.method === "submit") {
            this.submits.push({ connection, message });
            connection.peer.send({
                id: message.id,
                jsonrpc: "2.0",
                error: null,
                result: { status: "OK" }
            });
            return;
        }

        if (message.method === "keepalived") {
            connection.peer.send({
                id: message.id,
                jsonrpc: "2.0",
                error: null,
                result: { status: "KEEPALIVED" }
            });
        }
    }

    nextJob() {
        const seq = ++this.jobSeq;
        const suffix = seq.toString(16).padStart(8, "0");

        return {
            blob: BASE_BLOB.slice(0, -8) + suffix,
            job_id: `offline-job-${seq}`,
            target: "b88d0600",
            algo: "cn-heavy/xhv",
            height: 1000 + seq
        };
    }

    sendJob(connection, overrides = {}) {
        const job = Object.assign(this.nextJob(), overrides);

        connection.peer.send({
            jsonrpc: "2.0",
            method: "job",
            params: job
        });

        return job;
    }

    broadcastJob(overrides = {}) {
        return this.connections.map(connection => this.sendJob(connection, overrides));
    }

    async waitForLogins(count) {
        await waitFor(() => this.logins.length >= count, this.timeoutMs, `${count} upstream login request(s)`);
    }

    async waitForGetjobs(count) {
        await waitFor(() => this.getjobs.length >= count, this.timeoutMs, `${count} upstream getjob request(s)`);
    }

    async waitForSubmits(count) {
        await waitFor(() => this.submits.length >= count, this.timeoutMs, `${count} upstream submit request(s)`);
    }

    async close() {
        for (const connection of this.connections) {
            connection.peer.close();
        }

        await new Promise(resolve => this.server.close(resolve));
    }
}

class FakeMiner {
    constructor(name, port, timeoutMs) {
        this.name = name;
        this.port = port;
        this.timeoutMs = timeoutMs;
        this.socket = null;
        this.peer = null;
        this.lastJob = null;
    }

    async connect() {
        const started = Date.now();

        while (Date.now() - started < this.timeoutMs) {
            try {
                await new Promise((resolve, reject) => {
                    const socket = net.connect({ host: "127.0.0.1", port: this.port });
                    socket.once("connect", () => {
                        this.socket = socket;
                        this.peer = new JsonPeer(socket, this.name);
                        this.peer.on("message", message => {
                            if (message.method === "job" && message.params) {
                                this.lastJob = message.params;
                            }
                        });
                        resolve();
                    });
                    socket.once("error", reject);
                });
                return;
            }
            catch (error) {
                await delay(50);
            }
        }

        throw new Error(`${this.name} failed to connect to proxy on port ${this.port}`);
    }

    async login(capabilities = {}) {
        assert.ok(this.peer, `${this.name} is not connected`);
        const params = Object.assign({
            login: this.name,
            pass: "x",
            agent: "offline-proxy-test"
        }, capabilities.params || {});

        if (Object.prototype.hasOwnProperty.call(capabilities, "algos")) {
            params.algo = capabilities.algos;
        }

        if (Object.prototype.hasOwnProperty.call(capabilities, "perfs")) {
            params["algo-perf"] = capabilities.perfs;
        }

        this.peer.send({
            id: 1,
            jsonrpc: "2.0",
            method: "login",
            params: params
        });

        const response = await this.peer.waitForMessage(
            message => message.id === 1,
            this.timeoutMs,
            `${this.name} login response`
        );

        assert.equal(response.error, null, `${this.name} login returned an error`);
        assert.ok(response.result && response.result.job, `${this.name} login response does not contain a job`);
        this.lastJob = response.result.job;
    }

    async waitForJob(predicate, description) {
        const message = await this.peer.waitForMessage(
            candidate => candidate.method === "job" && candidate.params && (!predicate || predicate(candidate.params)),
            this.timeoutMs,
            description || `${this.name} job notification`
        );

        this.lastJob = message.params;
        return message.params;
    }

    close() {
        if (this.peer) {
            this.peer.close();
        }
    }
}

function proxyLogTail(child) {
    if (!child || !child.output) {
        return "";
    }

    return child.output.join("").split(/\r?\n/).slice(-80).join("\n");
}

function spawnProxy(binary, poolPort, proxyPort, config, options = {}) {
    const args = [
        "--no-color",
        "--verbose",
        "--donate-level=0",
        "--mode=nicehash",
        "--retries=1",
        "--retry-pause=1",
        "--algo-perf-same-threshold=20",
        `--url=127.0.0.1:${poolPort}`,
        "--user=offline-wallet",
        "--pass=x",
        `--bind=127.0.0.1:${proxyPort}`
    ].concat(options.proxyArgs || []);

    const child = spawn(binary, args, {
        cwd: options.cwd,
        env: process.env,
        windowsHide: true
    });

    const output = [];
    const capture = stream => {
        stream.on("data", chunk => {
            const text = chunk.toString();
            output.push(text);
            if (config.verbose) {
                process.stdout.write(text);
            }
        });
    };

    capture(child.stdout);
    capture(child.stderr);

    child.output = output;
    child.exited = false;
    child.once("exit", (code, signal) => {
        child.exited = true;
        child.exitCodeValue = code;
        child.exitSignalValue = signal;
    });

    return child;
}

async function stopProxy(child) {
    if (!child || child.exited) {
        return;
    }

    child.kill("SIGTERM");

    for (let i = 0; i < 20; ++i) {
        if (child.exited) {
            return;
        }

        await delay(100);
    }

    if (process.platform === "win32") {
        spawnSync("taskkill", ["/pid", String(child.pid), "/T", "/F"], { stdio: "ignore" });
    }
    else {
        child.kill("SIGKILL");
    }
}

async function withProxy(testFn, options = {}) {
    const config = getTestConfig();
    const pool = new FakePool(config.timeoutMs);
    const miners = [];
    const proxyCwd = fs.mkdtempSync(path.join(os.tmpdir(), "xmrig-proxy-test-"));
    let proxy = null;

    await pool.start();
    const proxyPort = await getFreePort();

    try {
        proxy = spawnProxy(config.binary, pool.port, proxyPort, config, Object.assign({ cwd: proxyCwd }, options));
        await pool.waitForLogins(1);

        const addMiner = async (name, capabilities) => {
            const miner = new FakeMiner(name, proxyPort, config.timeoutMs);
            miners.push(miner);
            await miner.connect();
            await miner.login(capabilities);
            return miner;
        };

        await testFn({
            addMiner,
            config,
            miners,
            pool,
            proxy,
            proxyPort
        });
    }
    catch (error) {
        const tail = proxyLogTail(proxy);
        if (tail) {
            console.error("\n--- xmrig-proxy log tail ---");
            console.error(tail);
            console.error("--- end xmrig-proxy log tail ---\n");
        }

        throw error;
    }
    finally {
        for (const miner of miners) {
            miner.close();
        }

        await stopProxy(proxy);
        await pool.close();
        fs.rmSync(proxyCwd, { force: true, recursive: true });
    }
}

function assertSetEqual(actual, expected, label) {
    assert.deepEqual([...actual].sort(), [...expected].sort(), label);
}

function assertHasAlgoPayload(request, label) {
    assert.ok(request.params, `${label} missing params`);
    assert.ok(Array.isArray(request.params.algo), `${label} missing algo array`);
    assert.ok(request.params["algo-perf"] && typeof request.params["algo-perf"] === "object", `${label} missing algo-perf object`);
}

function assertPerfValues(request, expected, label) {
    assertHasAlgoPayload(request, label);

    for (const [algo, value] of Object.entries(expected)) {
        assert.equal(request.params["algo-perf"][algo], value, `${label}: ${algo}`);
    }
}

function assertAlgoPayload(request, algos, perfs, label) {
    assertHasAlgoPayload(request, label);
    assertSetEqual(request.params.algo, algos, `${label}: algo set`);
    assertSetEqual(Object.keys(request.params["algo-perf"]), algos, `${label}: algo-perf keys`);

    if (perfs) {
        assertPerfValues(request, perfs, `${label}: algo-perf values`);
    }
}

module.exports = {
    CAPABILITIES,
    DEFAULT_ALGOS,
    DEFAULT_PERFS,
    FakeMiner,
    FakePool,
    assertAlgoPayload,
    assertHasAlgoPayload,
    assertPerfValues,
    assertSetEqual,
    delay,
    waitFor,
    withProxy
};
