"use strict";

const test = require("node:test");

const {
    CAPABILITIES,
    assertAlgoPayload,
    withProxy
} = require("./common/proxy_harness.js");

test.describe("MoneroOcean config edge cases", { concurrency: false }, () => {
    test("negative algo-perf-same-threshold is ignored instead of becoming a huge unsigned tolerance", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-base", CAPABILITIES.base);
            await pool.waitForGetjobs(1);

            const outlier = {
                algos: ["rx/0", "cn-heavy/xhv"],
                perfs: {
                    "rx/0": 2500,
                    "cn-heavy/xhv": 10
                }
            };

            await addMiner("miner-outlier", outlier);
            await pool.waitForLogins(2);
            assertAlgoPayload(pool.logins[1].message, outlier.algos, outlier.perfs, "outlier with negative threshold upstream login");
        }, {
            proxyArgs: ["--algo-perf-same-threshold=-1"]
        });
    });
});
