if (WITH_TLS)
    set(OPENSSL_ROOT_DIR ${XMRIG_DEPS})

    if (WIN32)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
        set(OPENSSL_MSVC_STATIC_RT TRUE)

        set(EXTRA_LIBS ${EXTRA_LIBS} Crypt32)
    endif()

    find_package(OpenSSL)

    if (OPENSSL_FOUND)
        set(TLS_SOURCES
            src/common/net/Tls.cpp
            src/common/net/Tls.h
            src/proxy/tls/Tls.cpp
            src/proxy/tls/Tls.h
            src/proxy/tls/TlsConfig.cpp
            src/proxy/tls/TlsConfig.h
            src/proxy/tls/TlsContext.cpp
            src/proxy/tls/TlsContext.h
            )
        include_directories(${OPENSSL_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_TLS=OFF` to build without TLS support")
    endif()
else()
    set(TLS_SOURCES "")
    set(OPENSSL_LIBRARIES "")
    add_definitions(/DXMRIG_NO_TLS)

    set(CMAKE_PROJECT_NAME "${CMAKE_PROJECT_NAME}-notls")
endif()
