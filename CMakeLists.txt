cmake_minimum_required(VERSION 2.8.12)
project(xmrig-proxy)

option(WITH_TLS             "Enable TLS (Transport Layer Security) support" ON)
option(WITH_HTTP            "Enable HTTP protocol support (client/server)" ON)
option(WITH_DEBUG_LOG       "Enable debug log output" OFF)
option(WITH_CRYPTONOTE      "Enable basic CryptoNote tools (block template and wallet address parsers)" ON)
option(WITH_CRYPTO_OPS      "Enable miner signatures support (Wownero)" ON)
option(WITH_CN_LITE         "Enable CryptoNight-Lite algorithms family" ON)
option(WITH_CN_HEAVY        "Enable CryptoNight-Heavy algorithms family" ON)
option(WITH_CN_PICO         "Enable CryptoNight-Pico algorithm" ON)
option(WITH_CN_FEMTO        "Enable CryptoNight-UPX2 algorithm" ON)
option(WITH_RANDOMX         "Enable RandomX algorithms family" ON)
option(WITH_ARGON2          "Enable Argon2 algorithms family" ON)
option(WITH_ASTROBWT        "Enable AstroBWT algorithms family" ON)
option(WITH_ENV_VARS        "Enable environment variables support in config file" ON)
option(WITH_GOOGLE_BREAKPAD "Use Google Breakpad" OFF)

add_definitions(-DXMRIG_PROXY_PROJECT -DXMRIG_FORCE_TLS -DAPP_DEVEL)

set(WITH_SECURE_JIT OFF)
set(WITH_HWLOC      OFF)
set(WITH_SODIUM     OFF)
set(WITH_CRYPTONOTE ON)
set(WITH_CRYPTO_OPS ON)
set(WITH_KAWPOW     OFF)
set(WITH_BENCHMARK  OFF)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/base/cmake")
set(LIBS base)

include(base)
add_subdirectory(src/base)

set(HEADERS
    src/App.h
    src/core/config/Config_platform.h
    src/core/config/Config.h
    src/core/config/ConfigTransform.h
    src/core/config/usage.h
    src/core/Controller.h
    src/donate.h
    src/net/JobResult.h
    src/net/strategies/DonateStrategy.h
    src/proxy/BindHost.h
    src/proxy/Counters.h
    src/proxy/CustomDiff.h
    src/proxy/Error.h
    src/proxy/Events.h
    src/proxy/events/AcceptEvent.h
    src/proxy/events/CloseEvent.h
    src/proxy/events/ConnectionEvent.h
    src/proxy/events/Event.h
    src/proxy/events/LoginEvent.h
    src/proxy/events/MinerEvent.h
    src/proxy/events/SubmitEvent.h
    src/proxy/interfaces/IEvent.h
    src/proxy/interfaces/IEventListener.h
    src/proxy/interfaces/ISplitter.h
    src/proxy/log/AccessLog.h
    src/proxy/log/ShareLog.h
    src/proxy/Login.h
    src/proxy/Miner.h
    src/proxy/Miners.h
    src/proxy/Proxy.h
    src/proxy/ProxyDebug.h
    src/proxy/Server.h
    src/proxy/splitters/donate/DonateMapper.h
    src/proxy/splitters/donate/DonateSplitter.h
    src/proxy/splitters/extra_nonce/ExtraNonceMapper.h
    src/proxy/splitters/extra_nonce/ExtraNonceSplitter.h
    src/proxy/splitters/extra_nonce/ExtraNonceStorage.h
    src/proxy/splitters/nicehash/NonceMapper.h
    src/proxy/splitters/nicehash/NonceSplitter.h
    src/proxy/splitters/nicehash/NonceStorage.h
    src/proxy/splitters/simple/SimpleMapper.h
    src/proxy/splitters/simple/SimpleSplitter.h
    src/proxy/splitters/Splitter.h
    src/proxy/Stats.h
    src/proxy/StatsData.h
    src/proxy/TickingCounter.h
    src/proxy/workers/Worker.h
    src/proxy/workers/Workers.h
    src/Summary.h
    src/version.h
   )

set(SOURCES
    src/App.cpp
    src/core/config/Config.cpp
    src/core/config/ConfigTransform.cpp
    src/core/Controller.cpp
    src/net/JobResult.cpp
    src/net/strategies/DonateStrategy.cpp
    src/proxy/BindHost.cpp
    src/proxy/Counters.cpp
    src/proxy/CustomDiff.cpp
    src/proxy/Error.cpp
    src/proxy/Events.cpp
    src/proxy/events/ConnectionEvent.h
    src/proxy/events/Event.cpp
    src/proxy/events/MinerEvent.cpp
    src/proxy/log/AccessLog.cpp
    src/proxy/log/ShareLog.cpp
    src/proxy/Login.cpp
    src/proxy/Miner.cpp
    src/proxy/Miners.cpp
    src/proxy/Proxy.cpp
    src/proxy/ProxyDebug.cpp
    src/proxy/Server.cpp
    src/proxy/splitters/donate/DonateMapper.cpp
    src/proxy/splitters/donate/DonateSplitter.cpp
    src/proxy/splitters/extra_nonce/ExtraNonceMapper.cpp
    src/proxy/splitters/extra_nonce/ExtraNonceSplitter.cpp
    src/proxy/splitters/extra_nonce/ExtraNonceStorage.cpp
    src/proxy/splitters/nicehash/NonceMapper.cpp
    src/proxy/splitters/nicehash/NonceSplitter.cpp
    src/proxy/splitters/nicehash/NonceStorage.cpp
    src/proxy/splitters/simple/SimpleMapper.cpp
    src/proxy/splitters/simple/SimpleSplitter.cpp
    src/proxy/splitters/Splitter.cpp
    src/proxy/Stats.cpp
    src/proxy/workers/Worker.cpp
    src/proxy/workers/Workers.cpp
    src/Summary.cpp
    src/xmrig.cpp
   )

if (XMRIG_OS_WIN)
    list(APPEND SOURCES res/app.rc)
endif()

if (WITH_GOOGLE_BREAKPAD)
    include_directories(/usr/local/include/breakpad)
    list(APPEND LIBS breakpad_client)
else()
    add_definitions(-DXMRIG_NO_GOOGLE_BREAKPAD)
endif()

if (WITH_HTTP)
    list(APPEND HEADERS src/api/v1/ApiRouter.h)
    list(APPEND SOURCES src/api/v1/ApiRouter.cpp)
endif()

if (WITH_TLS)
    list(APPEND HEADERS src/proxy/tls/MinerTls.h)
    list(APPEND SOURCES src/proxy/tls/MinerTls.cpp)
endif()

add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBS})
