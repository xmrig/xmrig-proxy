"use strict";

const test = require("node:test");

const {
    DEFAULT_ALGOS,
    DEFAULT_PERFS,
    assertAlgoPayload,
    withProxy
} = require("./common/proxy_harness.js");

test.describe("miner capability normalization", { concurrency: false }, () => {
    test("malformed algo and algo-perf entries are ignored while valid algos are retained", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-normalize", {
                algos: ["rx/0", 7, "invalid", "cn-heavy/xhv"],
                perfs: {
                    "rx/0": 900,
                    "cn-heavy/xhv": -5,
                    "cn/half": "not-a-number",
                    invalid: 11
                }
            });

            await pool.waitForGetjobs(1);
            assertAlgoPayload(pool.getjobs[0].message, ["rx/0", "cn-heavy/xhv"], {
                "rx/0": 900,
                "cn-heavy/xhv": 1
            }, "normalized miner getjob");
        });
    });

    test("algo-perf keys alone create the advertised algo set", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-perfs-only", {
                perfs: {
                    "rx/0": 1100,
                    "cn-heavy/xhv": 12
                }
            });

            await pool.waitForGetjobs(1);
            assertAlgoPayload(pool.getjobs[0].message, ["rx/0", "cn-heavy/xhv"], {
                "rx/0": 1100,
                "cn-heavy/xhv": 12
            }, "algo-perf-only miner getjob");
        });
    });

    test("algo array entries missing from algo-perf receive a conservative default perf", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-missing-perfs", {
                algos: ["rx/0", "cn-heavy/xhv", "cn/half"],
                perfs: {
                    "rx/0": 800
                }
            });

            await pool.waitForGetjobs(1);
            assertAlgoPayload(pool.getjobs[0].message, ["rx/0", "cn-heavy/xhv", "cn/half"], {
                "rx/0": 800,
                "cn-heavy/xhv": 1,
                "cn/half": 1
            }, "missing algo-perf defaults getjob");
        });
    });

    test("duplicate and reordered algo entries normalize to a stable capability set", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-duplicates", {
                algos: ["cn-heavy/xhv", "rx/0", "rx/0", "cn-heavy/xhv"],
                perfs: {
                    "cn-heavy/xhv": 12,
                    "rx/0": 1100
                }
            });

            await pool.waitForGetjobs(1);
            assertAlgoPayload(pool.getjobs[0].message, ["rx/0", "cn-heavy/xhv"], {
                "rx/0": 1100,
                "cn-heavy/xhv": 12
            }, "duplicate/reordered miner getjob");
        });
    });

    test("invalid-only capability data falls back to default MoneroOcean capabilities", async () => {
        await withProxy(async ({ addMiner, pool }) => {
            await addMiner("miner-invalid-only", {
                algos: ["unknown", 42, null],
                perfs: {
                    unknown: 100,
                    "bad/algo": 5,
                    "rx/0": -1
                }
            });

            await pool.waitForGetjobs(1);
            assertAlgoPayload(pool.getjobs[0].message, DEFAULT_ALGOS, DEFAULT_PERFS, "invalid-only miner getjob");
        });
    });
});
