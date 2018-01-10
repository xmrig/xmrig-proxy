cmake_minimum_required(VERSION 2.8)
project(xmrig-proxy)


option(WITH_GOOGLE_BREAKPAD "Use Google Breakpad" OFF)
option(WITH_HTTPD           "HTTP REST API"       ON)


include (CheckIncludeFile)


set(HEADERS
    src/3rdparty/align.h
    src/api/Api.h
    src/api/ApiState.h
    src/App.h
    src/Console.h
    src/donate.h
    src/interfaces/IClientListener.h
    src/interfaces/IConsoleListener.h
    src/interfaces/IEvent.h
    src/interfaces/IEventListener.h
    src/interfaces/ILogBackend.h
    src/interfaces/IStrategy.h
    src/interfaces/IStrategyListener.h
    src/log/AccessLog.h
    src/log/ConsoleLog.h
    src/log/FileLog.h
    src/log/Log.h
    src/log/ShareLog.h
    src/net/Client.h
    src/net/Job.h
    src/net/JobId.h
    src/net/strategies/DonateStrategy.h
    src/net/strategies/FailoverStrategy.h
    src/net/strategies/SinglePoolStrategy.h
    src/net/Url.h
    src/Options.h
    src/Platform.h
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
    src/proxy/JobResult.h
    src/proxy/Miner.h
    src/proxy/Miners.h
    src/proxy/Proxy.h
    src/proxy/ProxyDebug.h
    src/proxy/Server.h
    src/proxy/splitters/NonceMapper.h
    src/proxy/splitters/NonceSplitter.h
    src/proxy/splitters/NonceStorage.h
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
    src/api/Api.cpp
    src/api/ApiState.cpp
    src/App.cpp
    src/Console.cpp
    src/crypto/c_keccak.c
    src/crypto/c_keccak.h
    src/log/AccessLog.cpp
    src/log/ConsoleLog.cpp
    src/log/FileLog.cpp
    src/log/Log.cpp
    src/log/ShareLog.cpp
    src/net/Client.cpp
    src/net/Job.cpp
    src/net/strategies/DonateStrategy.cpp
    src/net/strategies/FailoverStrategy.cpp
    src/net/strategies/SinglePoolStrategy.cpp
    src/net/SubmitResult.cpp
    src/net/Url.cpp
    src/Options.cpp
    src/Platform.cpp
    src/proxy/Counters.cpp
    src/proxy/CustomDiff.cpp
    src/proxy/Error.cpp
    src/proxy/Events.cpp
    src/proxy/events/ConnectionEvent.h
    src/proxy/events/Event.cpp
    src/proxy/JobResult.cpp
    src/proxy/LoginRequest.cpp
    src/proxy/Miner.cpp
    src/proxy/Miners.cpp
    src/proxy/Proxy.cpp
    src/proxy/ProxyDebug.cpp
    src/proxy/Server.cpp
    src/proxy/splitters/NonceMapper.cpp
    src/proxy/splitters/NonceSplitter.cpp
    src/proxy/splitters/NonceStorage.cpp
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
        src/Platform_win.cpp
        src/proxy/Uuid_win.cpp
        )

    add_definitions(/DWIN32)
    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv)
elseif (APPLE)
    set(SOURCES_OS
        src/App_unix.cpp
        src/Platform_mac.cpp
        src/proxy/Uuid_mac.cpp
        )

    find_library(CFLIB CoreFoundation)
    set(EXTRA_LIBS ${CFLIB})
else()
    set(SOURCES_OS
        src/App_unix.cpp
        src/Platform_unix.cpp
        src/proxy/Uuid_unix.cpp
        )

    set(EXTRA_LIBS pthread uuid)
endif()

add_definitions(/DXMRIG_PROXY_PROJECT)
add_definitions(/DUNICODE)
add_definitions(/D__STDC_FORMAT_MACROS)
add_definitions(/DAPP_DEVEL)
add_definitions(/DNDEBUG)
#add_definitions(/DAPP_DEBUG)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
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

# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
if (CMAKE_CXX_COMPILER_ID MATCHES GNU)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

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

CHECK_INCLUDE_FILE (syslog.h HAVE_SYSLOG_H)
if (HAVE_SYSLOG_H)
    add_definitions(/DHAVE_SYSLOG_H)
    set(SOURCES_SYSLOG src/log/SysLog.h src/log/SysLog.cpp)
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
        set(HTTPD_SOURCES src/api/Httpd.h src/api/Httpd.cpp)
    else()
        message(FATAL_ERROR "microhttpd NOT found: use `-DWITH_HTTPD=OFF` to build without http deamon support")
    endif()
else()
    add_definitions(/DXMRIG_NO_HTTPD)
    add_definitions(/DXMRIG_NO_API)
endif()

include_directories(src)
include_directories(src/3rdparty)
include_directories(${UV_INCLUDE_DIR})

add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES} ${SOURCES_OS} ${SOURCES_SYSLOG} ${HTTPD_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${UV_LIBRARIES} ${MHD_LIBRARY} ${EXTRA_LIBS} ${GOOGLE_BREAKPAD_LIBS})
