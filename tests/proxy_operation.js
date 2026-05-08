"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");

const {
    CAPABILITIES,
    DEFAULT_ALGOS,
    DEFAULT_PERFS,
    assertAlgoPayload,
    withProxy
} = require("./common/proxy_harness.js");

test.describe("proxy startup and default negotiation", { concurrency: false }, () => {
    test("upstream login advertises default MoneroOcean fallback capabilities before miners connect", async () => {
        await withProxy(async ({ pool }) => {
            assertAlgoPayload(pool.logins[0].message, DEFAULT_ALGOS, DEFAULT_PERFS, "initial upstream login");
            assert.equal(pool.getjobs.length, 0, "proxy should not send getjob before miner capability changes");
        });
    });

    test("miner without algo fields uses default MoneroOcean capabilities", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-defaults", {});

            await pool.waitForGetjobs(1);
            assertAlgoPayload(pool.getjobs[0].message, DEFAULT_ALGOS, DEFAULT_PERFS, "default miner getjob");
        });
    });

    test("pool job notifications are forwarded to the connected miner", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            const miner = await addMiner("miner-forward", CAPABILITIES.base);
            await pool.waitForGetjobs(1);

            const job = pool.sendJob(pool.connections[0], {
                job_id: "pushed-job-1",
                height: 3001
            });
            const forwarded = await miner.waitForJob(params => params.job_id === job.job_id, "forwarded pool job");

            assert.equal(forwarded.algo, job.algo);
            assert.equal(forwarded.height, 3001);
        });
    });
});
