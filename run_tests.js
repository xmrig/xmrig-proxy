#!/usr/bin/env node
"use strict";

const { spawnSync } = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");

const ROOT = __dirname;
const DEFAULT_TIMEOUT_MS = 15000;

function parseArgs(argv) {
    const options = {
        build: true,
        buildDir: path.join(ROOT, "build"),
        binary: null,
        cmakeArgs: [],
        cmakeGenerator: process.env.CMAKE_GENERATOR || "",
        timeoutMs: DEFAULT_TIMEOUT_MS,
        verbose: false
    };

    for (let i = 0; i < argv.length; ++i) {
        const arg = argv[i];
        const value = () => {
            if (i + 1 >= argv.length) {
                throw new Error(`missing value for ${arg}`);
            }
            return argv[++i];
        };

        if (arg === "--binary") {
            options.binary = path.resolve(value());
        }
        else if (arg.startsWith("--binary=")) {
            options.binary = path.resolve(arg.slice("--binary=".length));
        }
        else if (arg === "--build-dir") {
            options.buildDir = path.resolve(value());
        }
        else if (arg.startsWith("--build-dir=")) {
            options.buildDir = path.resolve(arg.slice("--build-dir=".length));
        }
        else if (arg === "--skip-build") {
            options.build = false;
        }
        else if (arg === "--cmake-generator") {
            options.cmakeGenerator = value();
        }
        else if (arg.startsWith("--cmake-generator=")) {
            options.cmakeGenerator = arg.slice("--cmake-generator=".length);
        }
        else if (arg === "--cmake-arg") {
            options.cmakeArgs.push(value());
        }
        else if (arg.startsWith("--cmake-arg=")) {
            options.cmakeArgs.push(arg.slice("--cmake-arg=".length));
        }
        else if (arg === "--timeout-ms") {
            options.timeoutMs = Number(value());
        }
        else if (arg.startsWith("--timeout-ms=")) {
            options.timeoutMs = Number(arg.slice("--timeout-ms=".length));
        }
        else if (arg === "--verbose") {
            options.verbose = true;
        }
        else if (arg === "--help" || arg === "-h") {
            printHelp();
            process.exit(0);
        }
        else {
            throw new Error(`unknown argument: ${arg}`);
        }
    }

    if (!Number.isFinite(options.timeoutMs) || options.timeoutMs <= 0) {
        throw new Error("--timeout-ms must be a positive number");
    }

    return options;
}

function printHelp() {
    console.log(`Usage: node run_tests.js [options]

Builds xmrig-proxy when needed, then runs offline integration suites with local
fake pools and fake miners. No external pool or miner connections are used.

Options:
  --binary PATH              use an existing xmrig-proxy binary
  --build-dir PATH           CMake build directory, default: build
  --skip-build               do not configure or build
  --cmake-generator NAME     CMake generator used when configuring
  --cmake-arg ARG            extra CMake configure argument, repeatable
  --timeout-ms N             operation timeout, default: ${DEFAULT_TIMEOUT_MS}
  --verbose                  print proxy stdout/stderr while tests run
`);
}

function run(command, args, options = {}) {
    console.log(`$ ${[command].concat(args).join(" ")}`);
    const result = spawnSync(command, args, {
        cwd: options.cwd || ROOT,
        env: options.env || process.env,
        stdio: "inherit",
        windowsHide: true
    });

    if (result.error) {
        throw result.error;
    }

    if (result.status !== 0) {
        throw new Error(`${command} exited with status ${result.status}`);
    }
}

function configureArgs(options) {
    const args = ["-S", ROOT, "-B", options.buildDir, "-DCMAKE_BUILD_TYPE=Release"];

    if (options.cmakeGenerator) {
        args.push("-G", options.cmakeGenerator);
    }

    if (process.env.XMRIG_DEPS) {
        args.push(`-DXMRIG_DEPS=${process.env.XMRIG_DEPS}`);
    }

    if (process.env.OPENSSL_ROOT_DIR) {
        args.push(`-DOPENSSL_ROOT_DIR=${process.env.OPENSSL_ROOT_DIR}`);
    }

    return args.concat(options.cmakeArgs);
}

function findBinary(buildDir) {
    const exe = process.platform === "win32" ? ".exe" : "";
    const names = [`xmrig-proxy${exe}`, "xmrig-proxy", `xmrig-proxy-notls${exe}`, "xmrig-proxy-notls"];
    const dirs = [
        buildDir,
        path.join(buildDir, "Release"),
        path.join(buildDir, "RelWithDebInfo"),
        path.join(buildDir, "Debug")
    ];

    for (const dir of dirs) {
        for (const name of names) {
            const file = path.join(dir, name);
            if (fs.existsSync(file)) {
                return file;
            }
        }
    }

    throw new Error(`xmrig-proxy binary not found under ${buildDir}`);
}

function buildProxy(options) {
    if (options.binary) {
        if (!fs.existsSync(options.binary)) {
            throw new Error(`binary not found: ${options.binary}`);
        }
        return options.binary;
    }

    if (options.build) {
        if (!fs.existsSync(path.join(options.buildDir, "CMakeCache.txt"))) {
            run("cmake", configureArgs(options));
        }

        const parallel = process.env.BUILD_PARALLEL || String(Math.max(1, os.cpus().length));
        run("cmake", ["--build", options.buildDir, "--config", "Release", "--parallel", parallel]);
    }

    return findBinary(options.buildDir);
}

function runNodeTests(binary, options) {
    const env = Object.assign({}, process.env, {
        XMRIG_PROXY_TEST_BINARY: binary,
        XMRIG_PROXY_TEST_TIMEOUT_MS: String(options.timeoutMs),
        XMRIG_PROXY_TEST_VERBOSE: options.verbose ? "1" : "0"
    });

    run(process.execPath, [
        "--test",
        "--test-reporter=./tests/common/spec_reporter.js",
        "--test-concurrency=1",
        "tests/all.js"
    ], { env: env });
}

function main() {
    const options = parseArgs(process.argv.slice(2));
    const binary = buildProxy(options);

    console.log(`testing ${binary}`);
    runNodeTests(binary, options);
}

try {
    main();
}
catch (error) {
    console.error(error.stack || error.message || String(error));
    process.exit(1);
}
