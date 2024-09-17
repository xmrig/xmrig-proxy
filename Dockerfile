# syntax=docker/dockerfile:latest
FROM scratch AS src
COPY . /src

FROM alpine:latest AS build
RUN --mount=type=bind,readwrite,from=src,source=/src,target=/src \
    apk add \
        alpine-sdk \
        clang \
        cmake \
        libuv-dev \
        libuv-static \
        lld \
        openssl-dev \
        openssl-libs-static \
        samurai && \
    cmake \
        -S /src \
        -B /src/build \
        -G Ninja \
        -D BUILD_STATIC=ON \
        -D CMAKE_BUILD_TYPE=Release \
        -D CMAKE_C_COMPILER=clang \
        -D CMAKE_CXX_COMPILER=clang++ \
        -D CMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -static -static-libgcc -static-libstdc++" \
        -D OPENSSL_USE_STATIC_LIBS=ON \
        -D WITH_HTTP=ON \
        -D WITH_TLS=ON && \
    cmake --build /src/build && \
    mv /src/build/xmrig-proxy /xmrig-proxy

FROM scratch AS runtime
COPY --from=build /xmrig-proxy /xmrig-proxy
ENTRYPOINT [ "/xmrig-proxy" ]
