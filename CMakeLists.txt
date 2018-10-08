cmake_minimum_required(VERSION 2.8)
project(xmrig-proxy)


option(WITH_GOOGLE_BREAKPAD "Use Google Breakpad"     OFF)
option(WITH_DEBUG_LOG       "Enable debug log output" OFF)
option(WITH_HTTPD           "HTTP REST API"           ON)
option(WITH_TLS             "Enable OpenSSL support"  ON)


include (CheckIncludeFile)


set(HEADERS
    src/3rdparty/align.h
    src/App.h
    src/common/config/CommonConfig.h
    src/common/config/ConfigLoader.h
    src/common/config/ConfigWatcher.h
    src/common/Console.h
    src/common/crypto/Algorithm.h
    src/common/crypto/keccak.h
    src/common/interfaces/IClientListener.h
    src/common/interfaces/IConfig.h
    src/common/interfaces/IConfigCreator.h
    src/common/interfaces/IConsoleListener.h
    src/common/interfaces/IControllerListener.h
    src/common/interfaces/ILogBackend.h
    src/common/interfaces/IStrategy.h
    src/common/interfaces/IStrategyListener.h
    src/common/interfaces/IWatcherListener.h
    src/common/log/BasicLog.h
    src/common/log/ConsoleLog.h
    src/common/log/FileLog.h
    src/common/log/Log.h
    src/common/net/Client.h
    src/common/net/Id.h
    src/common/net/Job.h
    src/common/net/Pool.h
    src/common/net/Storage.h
    src/common/net/strategies/FailoverStrategy.h
    src/common/net/strategies/SinglePoolStrategy.h
    src/common/Platform.h
    src/common/xmrig.h
    src/core/Config.h
    src/core/ConfigLoader_platform.h
    src/core/Controller.h
    src/donate.h
    src/interfaces/IEvent.h
    src/interfaces/IEventListener.h
    src/interfaces/ISplitter.h
    src/log/AccessLog.h
    src/log/ShareLog.h
    src/net/JobResult.h
    src/net/strategies/DonateStrategy.h
    src/proxy/Addr.h
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
    src/proxy/Login.h
    src/proxy/Miner.h
    src/proxy/Miners.h
    src/proxy/Proxy.h
    src/proxy/ProxyDebug.h
    src/proxy/Server.h
    src/proxy/splitters/nicehash/NonceMapper.h
    src/proxy/splitters/nicehash/NonceSplitter.h
    src/proxy/splitters/nicehash/NonceStorage.h
    src/proxy/splitters/simple/SimpleMapper.h
    src/proxy/splitters/simple/SimpleSplitter.h
    src/proxy/splitters/Splitter.h
    src/proxy/Stats.h
    src/proxy/StatsData.h
    src/proxy/TickingCounter.h
    src/proxy/Uuid.h
    src/proxy/workers/Worker.h
    src/proxy/workers/Workers.h
    src/Summary.h
    src/version.h
   )

set(SOURCES
    src/App.cpp
    src/common/config/CommonConfig.cpp
    src/common/config/ConfigLoader.cpp
    src/common/config/ConfigWatcher.cpp
    src/common/Console.cpp
    src/common/crypto/Algorithm.cpp
    src/common/crypto/keccak.cpp
    src/common/log/BasicLog.cpp
    src/common/log/ConsoleLog.cpp
    src/common/log/FileLog.cpp
    src/common/log/Log.cpp
    src/common/net/Client.cpp
    src/common/net/Job.cpp
    src/common/net/Pool.cpp
    src/common/net/strategies/FailoverStrategy.cpp
    src/common/net/strategies/SinglePoolStrategy.cpp
    src/common/net/SubmitResult.cpp
    src/common/Platform.cpp
    src/core/Config.cpp
    src/core/Controller.cpp
    src/log/AccessLog.cpp
    src/log/ShareLog.cpp
    src/net/JobResult.cpp
    src/net/strategies/DonateStrategy.cpp
    src/proxy/Counters.cpp
    src/proxy/CustomDiff.cpp
    src/proxy/Error.cpp
    src/proxy/Events.cpp
    src/proxy/events/ConnectionEvent.h
    src/proxy/events/Event.cpp
    src/proxy/events/MinerEvent.cpp
    src/proxy/Login.cpp
    src/proxy/Miner.cpp
    src/proxy/Miners.cpp
    src/proxy/Proxy.cpp
    src/proxy/ProxyDebug.cpp
    src/proxy/Server.cpp
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
        res/app.rc
        src/App_win.cpp
        src/common/Platform_win.cpp
        src/proxy/Uuid_win.cpp
        )

    add_definitions(/DWIN32)
    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv)
elseif (APPLE)
    set(SOURCES_OS
        src/App_unix.cpp
        src/common/Platform_mac.cpp
        src/proxy/Uuid_mac.cpp
        )

    find_library(CFLIB CoreFoundation)
    set(EXTRA_LIBS ${CFLIB})
else()
    set(SOURCES_OS
        src/App_unix.cpp
        src/common/Platform_unix.cpp
        src/proxy/Uuid_unix.cpp
        )

    if (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
        set(EXTRA_LIBS pthread)
    else()
        set(EXTRA_LIBS pthread uuid rt dl)
    endif()
endif()

add_definitions(/DXMRIG_PROXY_PROJECT)
add_definitions(/DUNICODE)
add_definitions(/D__STDC_FORMAT_MACROS)
add_definitions(/DAPP_DEVEL)
add_definitions(/DXMRIG_NO_ASM)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(/DRAPIDJSON_SSE2)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(UV REQUIRED)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(/DNDEBUG)
endif()

# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
if (CMAKE_CXX_COMPILER_ID MATCHES GNU)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-class-memaccess")

    if (WIN32)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()

    add_definitions(/D_GNU_SOURCE)

    #set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -gdwarf-2")

    if (WITH_GOOGLE_BREAKPAD)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    endif()

elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /GL")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /GL")
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/D_CRT_NONSTDC_NO_WARNINGS)
    add_definitions(/DNOMINMAX)

elseif (CMAKE_CXX_COMPILER_ID MATCHES Clang)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-overloaded-virtual")

endif()

include(cmake/OpenSSL.cmake)

CHECK_INCLUDE_FILE (syslog.h HAVE_SYSLOG_H)
if (HAVE_SYSLOG_H)
    add_definitions(/DHAVE_SYSLOG_H)
    set(SOURCES_SYSLOG src/common/log/SysLog.h src/common/log/SysLog.cpp)
endif()

if (WITH_GOOGLE_BREAKPAD)
    include_directories(/usr/local/include/breakpad)
    set(GOOGLE_BREAKPAD_LIBS breakpad_client)
else()
    add_definitions(/DXMRIG_NO_GOOGLE_BREAKPAD)
endif()

if (WITH_HTTPD)
    find_package(MHD)

    if (MHD_FOUND)
        include_directories(${MHD_INCLUDE_DIRS})
        set(HTTPD_SOURCES
            src/api/Api.h
            src/api/ApiRouter.h
            src/common/api/HttpBody.h
            src/common/api/Httpd.h
            src/common/api/HttpReply.h
            src/common/api/HttpRequest.h
            src/api/Api.cpp
            src/api/ApiRouter.cpp
            src/common/api/Httpd.cpp
            src/common/api/HttpRequest.cpp
            )
    else()
        message(FATAL_ERROR "microhttpd NOT found: use `-DWITH_HTTPD=OFF` to build without http deamon support")
    endif()
else()
    set(HTTPD_SOURCES "")
    set(MHD_LIBRARY "")
    add_definitions(/DXMRIG_NO_HTTPD)
    add_definitions(/DXMRIG_NO_API)
endif()

if (WITH_DEBUG_LOG)
    add_definitions(/DAPP_DEBUG)
endif()

include_directories(src)
include_directories(src/3rdparty)
include_directories(${UV_INCLUDE_DIR})

add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES} ${SOURCES_OS} ${SOURCES_SYSLOG} ${HTTPD_SOURCES} ${TLS_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${OPENSSL_LIBRARIES} ${UV_LIBRARIES} ${MHD_LIBRARY} ${EXTRA_LIBS} ${GOOGLE_BREAKPAD_LIBS})
