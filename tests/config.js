"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const http = require("node:http");
const net = require("node:net");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");

const {
    CAPABILITIES,
    assertAlgoPayload,
    waitFor,
    withProxy
} = require("./common/proxy_harness.js");

async function freePort() {
    const server = net.createServer();
    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", resolve);
    });
    const port = server.address().port;
    await new Promise(resolve => server.close(resolve));
    return port;
}

function configRequest(port, timeoutMs, method = "GET", config) {
    return new Promise((resolve, reject) => {
        const request = http.request({
            host: "127.0.0.1", port, path: "/2/config", method,
            headers: { authorization: "Bearer offline-ip-ban-test", "content-type": "application/json" }
        }, response => {
            let body = "";
            response.setEncoding("utf8");
            response.on("data", chunk => { body += chunk; });
            response.on("end", () => resolve({ status: response.statusCode, body }));
            response.on("error", reject);
        });
        request.setTimeout(timeoutMs, () => request.destroy(new Error("config API request timed out")));
        request.on("error", reject);
        request.end(config === undefined ? undefined : JSON.stringify(config));
    });
}

async function expectInvalidIpBan(proxyArgs, label) {
    await withProxy(async ({ config, proxy }) => {
        await waitFor(() => proxy.exited, config.timeoutMs, `${label} config rejection`);
        assert.notEqual(proxy.exitCodeValue, 0, `${label} started with an invalid IP ban duration`);
        assert.ok(proxy.output.join("").includes("ip-ban-hours must be an integer between 0 and 4294967295"),
            `${label} did not report the invalid IP ban duration`);
    }, { proxyArgs: [...proxyArgs, "--dry-run"], waitForInitialPoolLogin: false });
}

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

    test("malformed CLI IP ban hours cannot accidentally disable bans", async () => {
        for (const value of ["", "-1", "+1", "1.5", "1h", " 0", "0 ", "0x0", "garbage", "4294967296", "18446744073709551616"]) {
            await expectInvalidIpBan([`--ip-ban-hours=${value}`], `CLI ${JSON.stringify(value)}`);
        }
    });

    test("malformed JSON IP ban hours cannot accidentally disable bans", async () => {
        const dir = fs.mkdtempSync(path.join(os.tmpdir(), "proxy-invalid-ip-ban-"));
        const configPath = path.join(dir, "config.json");

        try {
            for (const value of [-1, 1.5, "0", false, true, 4294967296, [], {}]) {
                fs.writeFileSync(configPath, JSON.stringify({ "ip-ban-hours": value, watch: false }));
                await expectInvalidIpBan(["--config", configPath], `JSON ${JSON.stringify(value)}`);
            }
        }
        finally {
            fs.rmSync(dir, { recursive: true, force: true });
        }
    });

    test("effective IP ban hours survive config serialization and saving", async () => {
        const variants = [
            { name: "default", expected: 24 },
            { name: "CLI disabled", args: ["--ip-ban-hours=0"], expected: 0 },
            { name: "CLI custom", args: ["--ip-ban-hours=0007"], expected: 7 },
            { name: "CLI maximum", args: ["--ip-ban-hours=4294967295"], expected: 4294967295 },
            { name: "JSON custom", hours: 2, expected: 2 }
        ];

        for (const variant of variants) {
            const dir = fs.mkdtempSync(path.join(os.tmpdir(), "proxy-saved-ip-ban-"));
            const configPath = path.join(dir, "config.json");
            const port = await freePort();
            const input = {
                watch: false,
                http: { enabled: true, host: "127.0.0.1", port, "access-token": "offline-ip-ban-test", restricted: false }
            };
            if (variant.hours !== undefined) input["ip-ban-hours"] = variant.hours;
            fs.writeFileSync(configPath, JSON.stringify(input));

            try {
                await withProxy(async ({ config }) => {
                    const response = await configRequest(port, config.timeoutMs);
                    assert.equal(response.status, 200, `${variant.name} config API response`);
                    const effective = JSON.parse(response.body);
                    assert.equal(effective["ip-ban-hours"], variant.expected, `${variant.name} effective config`);
                    const saved = await configRequest(port, config.timeoutMs, "PUT", effective);
                    assert.equal(saved.status, 204, `${variant.name} config save response`);
                    const persisted = JSON.parse(fs.readFileSync(configPath, "utf8"));
                    assert.equal(persisted["ip-ban-hours"], variant.expected, `${variant.name} saved config`);
                }, { proxyArgs: ["--config", configPath, ...(variant.args || [])] });
            }
            finally {
                fs.rmSync(dir, { recursive: true, force: true });
            }
        }
    });
});
