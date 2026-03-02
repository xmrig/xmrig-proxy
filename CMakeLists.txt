cmake_minimum_required(VERSION 3.10)
project(xmrig-proxy)


option(WITH_GOOGLE_BREAKPAD "Use Google Breakpad"     OFF)
option(WITH_DEBUG_LOG       "Enable debug log output" OFF)
option(WITH_HTTP            "HTTP protocol support (client/server)" ON)
option(WITH_TLS             "Enable OpenSSL support"  ON)
option(WITH_ENV_VARS        "Enable environment variables support in config file" ON)


include(CheckIncludeFile)
include(cmake/cpu.cmake)
include(cmake/os.cmake)
include(src/base/base.cmake)


set(HEADERS
    "${HEADERS_BASE}"
    "${HEADERS_BASE_HTTP}"
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
    "${SOURCES_BASE}"
    "${SOURCES_BASE_HTTP}"
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

if (WIN32)
    set(SOURCES_OS
        "${SOURCES_OS}"
        res/app.rc
        src/App_win.cpp
        )

    add_definitions(/DWIN32)
    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv dbghelp)
elseif (APPLE)
    set(SOURCES_OS
        "${SOURCES_OS}"
        src/App_unix.cpp
        )

    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORESERVICES_LIBRARY CoreServices)
    set(EXTRA_LIBS ${IOKIT_LIBRARY} ${CORESERVICES_LIBRARY})
else()
    set(SOURCES_OS
        "${SOURCES_OS}"
        src/App_unix.cpp
        )

    if (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
        set(EXTRA_LIBS pthread)
    else()
        set(EXTRA_LIBS pthread rt dl)
    endif()
endif()

add_definitions(/DXMRIG_PROXY_PROJECT /DXMRIG_FORCE_TLS)
add_definitions(/DUNICODE)
add_definitions(/D__STDC_FORMAT_MACROS)
add_definitions(/DAPP_DEVEL)
add_definitions(/DXMRIG_NO_ASM)

add_definitions(/DXMRIG_ALGO_RANDOMX)
add_definitions(/DXMRIG_ALGO_CN_LITE)
add_definitions(/DXMRIG_ALGO_CN_HEAVY)
add_definitions(/DXMRIG_ALGO_CN_PICO)
add_definitions(/DXMRIG_ALGO_CN_FEMTO)
add_definitions(/DXMRIG_ALGO_ARGON2)
add_definitions(/DXMRIG_ALGO_ASTROBWT)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(/DRAPIDJSON_SSE2)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(UV REQUIRED)

include(cmake/flags.cmake)
include(cmake/OpenSSL.cmake)

if (WITH_GOOGLE_BREAKPAD)
    include_directories(/usr/local/include/breakpad)
    set(GOOGLE_BREAKPAD_LIBS breakpad_client)
else()
    add_definitions(/DXMRIG_NO_GOOGLE_BREAKPAD)
endif()

if (WITH_HTTP)
    set(HTTP_SOURCES
        src/api/v1/ApiRouter.cpp
        src/api/v1/ApiRouter.h
        )
else()
    set(HTTP_SOURCES "")
endif()

if (WITH_DEBUG_LOG)
    add_definitions(/DAPP_DEBUG)
endif()

include_directories(src)
include_directories(src/3rdparty)
include_directories(${UV_INCLUDE_DIR})

add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES} ${SOURCES_OS} ${SOURCES_SYSLOG} ${HTTP_SOURCES} ${TLS_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${OPENSSL_LIBRARIES} ${UV_LIBRARIES} ${EXTRA_LIBS} ${GOOGLE_BREAKPAD_LIBS})

if (CMAKE_CXX_COMPILER_ID MATCHES Clang AND CMAKE_BUILD_TYPE STREQUAL Release AND NOT CMAKE_GENERATOR STREQUAL Xcode)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>")
endif()
