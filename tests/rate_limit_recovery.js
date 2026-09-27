"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");

const {
    CAPABILITIES,
    delay,
    FakePool,
    waitFor,
    withProxy
} = require("./common/proxy_harness.js");

const UNSUPPORTED_ALGO_ERROR = "algo array must include at least one supported pool algo: no matching work";
const TEMPLATE_WAIT_ERROR = "No block template yet. Please wait.";
const RESTRICTIVE_CAPABILITIES = {
    algos: ["rx/0"],
    perfs: { "rx/0": 1000 }
};

class NullGetjobPool extends FakePool {
    constructor(timeoutMs, options = {}) {
        super(timeoutMs, options);
        this.nullGetjobs = 0;
    }

    onMessage(connection, message) {
        if (message.method !== "getjob" || this.nullGetjobs > 0) {
            super.onMessage(connection, message);
            return;
        }

        const job = this.nextJob();
        this.getjobs.push({ at: Date.now(), connection, message, job });
        ++this.nullGetjobs;

        const respond = () => connection.peer.send({
            id: message.id,
            jsonrpc: "2.0",
            error: null,
            result: null
        });

        if (this.options.getjobDelayMs) {
            setTimeout(respond, this.options.getjobDelayMs);
        }
        else {
            respond();
        }
    }
}

class AlgoRejectPool extends FakePool {
    constructor(timeoutMs, options = {}) {
        super(timeoutMs, options);
        this.rejectedGetjobs = 0;
    }

    onMessage(connection, message) {
        if (message.method !== "getjob" || this.rejectedGetjobs >= (this.options.rejectGetjobs || 0)) {
            super.onMessage(connection, message);
            return;
        }

        const job = this.nextJob();
        this.getjobs.push({ at: Date.now(), connection, message, job });
        ++this.rejectedGetjobs;

        const respond = () => connection.peer.send({
            id: message.id,
            jsonrpc: "2.0",
            error: { code: -1, message: UNSUPPORTED_ALGO_ERROR },
            result: null
        });

        if (this.options.getjobDelayMs) {
            setTimeout(respond, this.options.getjobDelayMs);
        }
        else {
            respond();
        }
    }
}

class TemplateEofPool extends FakePool {
    constructor(timeoutMs, options = {}) {
        super(timeoutMs, options);
        this.options = Object.assign({
            closeDelayMs: 10,
            errorDelayMs: 0,
            failLogin: false
        }, options);
    }

    onConnection(socket) {
        super.onConnection(socket);
        this.connections[this.connections.length - 1].templateFailure = false;
    }

    onMessage(connection, message) {
        if (message.method === "login" && this.options.failLogin) {
            this.logins.push({ at: Date.now(), connection, message, job: undefined });
            connection.peer.send({
                id: message.id,
                jsonrpc: "2.0",
                error: { code: -1, message: TEMPLATE_WAIT_ERROR },
                result: null
            });
            setTimeout(() => connection.peer.close(), this.options.closeDelayMs);
            return;
        }

        if (message.method !== "getjob" || connection.templateFailure) {
            super.onMessage(connection, message);
            return;
        }

        const job = this.nextJob();
        connection.templateFailure = true;
        this.getjobs.push({ at: Date.now(), connection, message, job });

        const respond = () => {
            if (connection.peer.closed) {
                return;
            }

            connection.peer.send({
                id: message.id,
                jsonrpc: "2.0",
                error: { code: -1, message: TEMPLATE_WAIT_ERROR },
                result: null
            });
            setTimeout(() => connection.peer.close(), this.options.closeDelayMs);
        };

        if (this.options.errorDelayMs > 0) {
            setTimeout(respond, this.options.errorDelayMs);
        }
        else {
            respond();
        }
    }
}

function outputLines(proxy) {
    return proxy.output.join("").split(/\r?\n/).filter(Boolean);
}

async function waitForLogLine(proxy, config, predicate, description) {
    await waitFor(() => outputLines(proxy).some(predicate), config.timeoutMs, description);
    return outputLines(proxy).find(predicate);
}

function parseClientTag(line, label) {
    const match = line.match(/\[upstream=(\d+) algo=([^\s]+) offered=([^\]]+)\]/);
    assert.ok(match, `${label} has no structured upstream context`);

    return {
        upstream: match[1],
        algo: match[2],
        offered: match[3] === "none" ? [] : match[3].split(",")
    };
}

function assertSafeClientContext(line, label) {
    assert.doesNotMatch(line, /password|pass=|algo-perf|perf(?:ormance)?/i, `${label} leaked credential/perf context`);
}

test.describe("upstream request pacing and reconnect recovery", { concurrency: false }, () => {
    test("coalesces capability changes while an ID-1 getjob response is in flight", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-base", CAPABILITIES.base);
            const secondMiner = addMiner("miner-superset", CAPABILITIES.superset);

            await pool.waitForGetjobs(2);
            await secondMiner;

            assert.equal(pool.logins.length, 1, "getjob response must not be replayed as login success");
            assert.equal(pool.getjobs.length, 2, "capability changes should be coalesced into one follow-up getjob");
        }, {
            poolOptions: { getjobDelayMs: 250 }
        });
    });

    test("retries a queued capability refresh after a null getjob result", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            const firstMiner = await addMiner("miner-null-base", CAPABILITIES.base);
            const initialJob = firstMiner.lastJob;
            const secondMiner = addMiner("miner-null-superset", CAPABILITIES.superset);

            await pool.waitForGetjobs(2);
            await secondMiner;

            await firstMiner.peer.waitForMessage(
                message => message.method === "job"
                    && message.params
                    && message.params.job_id === pool.getjobs[1].job.job_id,
                firstMiner.timeoutMs,
                "queued getjob job notification"
            );

            assert.equal(pool.nullGetjobs, 1, "the first getjob response must be null");
            assert.equal(pool.logins.length, 1, "null getjob response must not replay upstream login");
            assert.equal(
                firstMiner.peer.messages.filter(message => message.id === 1).length,
                1,
                "null getjob response must not create another miner login response"
            );
            assert.equal(
                firstMiner.lastJob.job_id,
                pool.getjobs[1].job.job_id,
                "the queued getjob must refresh the existing job after null response"
            );
            assert.notEqual(firstMiner.lastJob.job_id, initialJob.job_id, "follow-up getjob should deliver a refreshed job");
        }, {
            poolFactory: (timeoutMs, options) => new NullGetjobPool(timeoutMs, options),
            poolOptions: { getjobDelayMs: 250 }
        });
    });

    test("suppresses repeated identical unsupported-algo getjob retries during cooldown", async () => {
        await withProxy(async ({ addMiner, config, pool, proxy }) => {
            await addMiner("miner-rejected", CAPABILITIES.base);
            await pool.waitForGetjobs(1);
            await waitFor(
                () => proxy.output.join("").includes(UNSUPPORTED_ALGO_ERROR),
                config.timeoutMs,
                "unsupported-algo rejection"
            );
            const churnMiner = addMiner("miner-rejected-churn", CAPABILITIES.superset);
            await churnMiner;
            await delay(400);

            assert.equal(pool.rejectedGetjobs, 1, "the same rejected algo array must not churn getjob requests");
            assert.equal(pool.getjobs.length, 1, "the cooldown must suppress duplicate getjobs");
            assert.equal(pool.logins.length, 1, "a capability rejection must not reconnect the upstream");
        }, {
            poolFactory: (timeoutMs, options) => new AlgoRejectPool(timeoutMs, options),
            poolOptions: { rejectGetjobs: 100 }
        });
    });

    test("retries immediately when the rejected request is superseded by a changed algo array", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            const firstMiner = await addMiner("miner-rejected-base", CAPABILITIES.base);
            await pool.waitForGetjobs(1);

            const secondMiner = addMiner("miner-recovered-restrictive", RESTRICTIVE_CAPABILITIES);
            await pool.waitForGetjobs(2);
            await secondMiner;

            assert.equal(pool.rejectedGetjobs, 1, "only the original algo array should be rejected");
            assert.notDeepEqual(
                pool.getjobs[0].message.params.algo,
                pool.getjobs[1].message.params.algo,
                "the changed algo array must bypass the rejection cooldown"
            );
            await firstMiner.peer.waitForMessage(
                message => message.method === "job"
                    && message.params
                    && message.params.job_id === pool.getjobs[1].job.job_id,
                firstMiner.timeoutMs,
                "recovered getjob notification"
            );
        }, {
            poolFactory: (timeoutMs, options) => new AlgoRejectPool(timeoutMs, options),
            poolOptions: { rejectGetjobs: 1, getjobDelayMs: 250 }
        });
    });

    test("keeps ordinary getjob errors from creating a retry loop", async () => {
        await withProxy(async ({ addMiner, config, pool, proxy }) => {
            await addMiner("miner-ordinary-error", CAPABILITIES.base);
            await pool.waitForGetjobs(1);
            await waitFor(
                () => proxy.output.join("").includes("temporary getjob failure"),
                config.timeoutMs,
                "ordinary getjob error"
            );
            const secondMiner = addMiner("miner-ordinary-error-repeat", CAPABILITIES.superset);
            await pool.waitForGetjobs(2);
            await secondMiner;
            await delay(400);

            assert.equal(pool.getjobs.length, 2, "ordinary getjob errors keep the existing explicit-request behavior");
            assert.equal(pool.logins.length, 1, "ordinary getjob errors must not reconnect the upstream");
        }, {
            poolOptions: { getjobError: { code: -1, message: "temporary getjob failure" } }
        });
    });

    test("does not write queued getjobs after the pool closes the socket", async () => {
        await withProxy(async ({ addMiner, pool, proxy }) => {
            await addMiner("miner-close", CAPABILITIES.base);
            await pool.waitForGetjobs(1);
            await pool.waitForLogins(2);
            await delay(250);

            assert.equal(pool.getjobs.length, 1, "reconnect login should satisfy current capabilities");
            assert.doesNotMatch(proxy.output.join(""), /send failed, invalid state/);
        }, {
            poolOptions: { closeOnGetjob: true }
        });
    });

    test("logs none before the first job and only the last offered algorithms on template errors", async () => {
        await withProxy(async ({ config, pool, proxy }) => {
            const offered = pool.logins[0].message.params.algo;
            const errorLine = await waitForLogLine(
                proxy,
                config,
                line => line.includes(TEMPLATE_WAIT_ERROR),
                "template error context before first job"
            );
            const tag = parseClientTag(errorLine, "template error");

            assert.equal(tag.algo, "none", "a client without a job must log algo=none");
            assert.deepEqual(tag.offered, offered, "error context must retain the last sent algo array");
            assertSafeClientContext(errorLine, "template error");

            const eofLine = await waitForLogLine(
                proxy,
                config,
                line => line.includes("read error: \"end of file\""),
                "EOF context after template error"
            );
            const eofTag = parseClientTag(eofLine, "EOF context");
            assert.equal(eofTag.algo, tag.algo, "EOF context must retain the current job marker");
            assert.deepEqual(eofTag.offered, tag.offered, "EOF context must retain the last offered algo array");
            assert.equal(eofTag.upstream, tag.upstream, "EOF context must retain the same upstream id");
            assertSafeClientContext(eofLine, "EOF context");
        }, {
            poolFactory: timeout => new TemplateEofPool(timeout, {
                failLogin: true
            })
        });
    });

    test("rate-limits repeated template login errors and their EOFs", async () => {
        await withProxy(async ({ config, pool, proxy }) => {
            await pool.waitForLogins(2);
            await delay(250);

            const lines = outputLines(proxy);
            assert.equal(
                lines.filter(line => line.includes(TEMPLATE_WAIT_ERROR)).length,
                1,
                "repeated template login errors should be logged once per endpoint"
            );
            assert.equal(
                lines.filter(line => line.includes("read error: \"end of file\"")).length,
                1,
                "repeated EOFs following the template error should be logged once per endpoint"
            );
        }, {
            poolFactory: timeout => new TemplateEofPool(timeout, {
                failLogin: true,
                closeDelayMs: 10
            })
        });
    });

    test("rate-limits repeated paused-group reports for the same upstream error", async () => {
        await withProxy(async ({ addMiner, config, pool, proxy }) => {
            await addMiner("miner-paused-repeat", CAPABILITIES.base);
            await pool.waitForGetjobs(1);
            await waitFor(
                () => outputLines(proxy).some(line => line.includes("paused: no active upstream")),
                config.timeoutMs,
                "first paused-group report"
            );
            await pool.waitForLogins(2);
            await delay(250);

            assert.equal(
                outputLines(proxy).filter(line => line.includes("paused: no active upstream")).length,
                1,
                "the same paused group and close reason should be logged once per cooldown"
            );
        }, {
            poolFactory: timeout => new TemplateEofPool(timeout, {
                closeDelayMs: 10
            })
        });
    });

    test("keeps in-flight offered algorithms stable and identifies distinct nonempty groups", async () => {
        await withProxy(async ({ addMiner, config, pool, proxy }) => {
            await addMiner("miner-wide-inflight", CAPABILITIES.superset);
            await pool.waitForGetjobs(1);
            const offered = pool.getjobs[0].message.params.algo;

            await addMiner("miner-narrow-inflight", CAPABILITIES.base);
            assert.equal(pool.getjobs.length, 1, "the changed group must remain queued while getjob is in flight");

            const firstError = await waitForLogLine(
                proxy,
                config,
                line => line.includes(TEMPLATE_WAIT_ERROR),
                "first in-flight template error"
            );
            const firstTag = parseClientTag(firstError, "first template error");
            assert.deepEqual(firstTag.offered, offered, "context must use the last sent array, not the unsent group");
            assert.equal(firstTag.algo, "cn-heavy/xhv");
            assertSafeClientContext(firstError, "first template error");

            const firstPause = await waitForLogLine(
                proxy,
                config,
                line => line.includes("paused: no active upstream"),
                "first nonempty group pause"
            );
            assert.match(firstPause, /miners=2/);
            assert.match(firstPause, /last_error="end of file"/);

            await addMiner("miner-second-group", {
                algos: ["cn/half"],
                perfs: { "cn/half": 2 }
            });
            await waitFor(
                () => pool.logins.some(login => login.message.params
                    && Array.isArray(login.message.params.algo)
                    && login.message.params.algo.length === 1
                    && login.message.params.algo[0] === "cn/half"),
                config.timeoutMs,
                "second group login"
            );
            const secondGroupLogin = pool.logins.find(login => login.message.params
                && Array.isArray(login.message.params.algo)
                && login.message.params.algo.length === 1
                && login.message.params.algo[0] === "cn/half");
            secondGroupLogin.connection.peer.close();

            const firstGroup = firstPause.match(/group=(\d+)/)[1];
            const secondPause = await waitForLogLine(
                proxy,
                config,
                line => {
                    if (!line.includes("paused: no active upstream")) {
                        return false;
                    }

                    const match = line.match(/group=(\d+)/);
                    return match && match[1] !== firstGroup;
                },
                "second nonempty group pause"
            );
            const firstFields = firstPause.match(/group=(\d+) miners=(\d+) .* upstream=(\d+)/);
            const secondFields = secondPause.match(/group=(\d+) miners=(\d+) .* upstream=(\d+)/);
            assert.ok(firstFields && secondFields, "pause lines have group/miner/upstream fields");
            assert.notEqual(firstFields[1], secondFields[1], "distinct mapper groups must have distinct ids");
            assert.notEqual(firstFields[3], secondFields[3], "distinct clients must have distinct upstream ids");
            assert.equal(firstFields[3], firstTag.upstream, "pause context must retain the failing client id");
            assert.ok(Number(firstFields[2]) > 0 && Number(secondFields[2]) > 0,
                "pause context must retain nonzero miner counts");
            assert.match(secondPause, /last_error="end of file"/);
        }, {
            poolFactory: timeout => new TemplateEofPool(timeout, { errorDelayMs: 250 })
        });
    });
});
