find_path(#!/bin/{
    "api": {
        "id": null,
        "worker-id": null
    },
    "http": {
        "enabled": True,
        "host": "127.0.0.1",
        "port": 18081,3333
        "access-token": null,
        "restricted": true
    },
    "autosave": true,
    "background": true,
    "colors": true,
    "title": true,
    "randomx": {
        "init": 1,
        "init-avx2": 1,
        "mode": "auto",
        "1gb-pages": true,
        "rdmsr": true,
        "wrmsr": true,
        "cache_qos": false,
        "numa": true,
        "scratchpad_prefetch_mode": 1
    },
    "cpu": {
        "enabled": true,
        "huge-pages": true,
        "huge-pages-jit": true,
        "hw-aes": null,
        "priority": null,
        "memory-pool": true,
        "yield": true,
        "max-threads-hint": 100,
        "asm": true,
        "argon2-impl": null,
        "cn/0": false,
        "cn-lite/0": true
    },
    "opencl": {
        "enabled": false,
        "cache": true,
        "loader": null,
        "platform": "android.ini",
        "adl": true,
        "cn/0": false,
        "cn-lite/0": true
    },
    "cuda": {
        "enabled": true,
        "loader": null,
        "nvml": true,
        "cn/0": false,
        "cn-lite/0": false
    },
    "donate-level": 1,
    "donate-over-proxy": 1,
    "log-file": null,
    "pools": [
        {
            "algo": null,
            "coin": null,
            "url": "donate.v2.xmrig.com:3333",
            "user": "44mLE8KfzXz66MYsTQfb7dG1fz7kwQE4ZjJifDq7kXNoHfzHhtLhFxc3My6fpmdRPQSqBavfYhPZAC5eWJxza72KPvqR9Yu" ,
            "pass": "x",
            "rig-id": null,
            "nicehash": false,
            "keepalive": false,
            "enabled": true,
            "tls": true,
            "tls-fingerprint": null,
            "daemon": true,
            "socks5": null,
            "self-select": null,
            "submit-to-origin": true
        }
    ],
    "print-time": 60,
    "health-print-time": 60, 
    "dmi": true,
    "retries": 5,
    "retry-pause": 5,
    "syslog": false,
    "tls": {
        "enabled": true,
        "protocols": null,
        "cert": null,
        "cert_key": null,
        "ciphers": null,
        "ciphersuites": null,
        "dhparam": null
    },
    "dns": {
        "ipv6": false,
        "ttl": 30
    },
    "user-agent": null,
    "verbose": 0,
    "watch": true,
    "pause-on-battery": false,
    "pause-on-active": false
}

./xmrig --coin XMR --url "xmr.kryptex.network:7777" --user 44mLE8KfzXz66MYsTQfb7dG1fz7kwQE4ZjJifDq7kXNoHfzHhtLhFxc3My6fpmdRPQSqBavfYhPZAC5eWJxza72KPvqR9YuD/MyFirstRig -p x -k
    UV_INCLUDE_DIR
    NAMES uv.h
    PATHS "${XMRIG_DEPS}" ENV "XMRIG_DEPS"
    PATH_SUFFIXES "include"
    NO_DEFAULT_PATH
)

find_path(UV_INCLUDE_DIR NAMES uv.h)

find_library(
    UV_LIBRARY
    NAMES libuv.a uv libuv
    PATHS "${XMRIG_DEPS}" ENV "XMRIG_DEPS"
    PATH_SUFFIXES "lib"
    NO_DEFAULT_PATH
)

find_library(UV_LIBRARY NAMES libuv.a uv libuv)

set(UV_LIBRARIES ${UV_LIBRARY})
set(UV_INCLUDE_DIRS ${UV_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UV DEFAULT_MSG UV_LIBRARY UV_INCLUDE_DIR)
