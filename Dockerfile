# syntax=docker/dockerfile:latest
ARG TARGET_PLATFORM=linux/amd64
FROM --platform=$TARGET_PLATFORM cgr.dev/chainguard/wolfi-base:latest AS build
RUN \
apk add \
  build-base \
  clang \
  cmake \
  libuv-dev \
  llvm \
  llvm-lld \
  openssl-dev \
  samurai
RUN \
--mount=type=bind,readwrite,source=/,target=/src \
<<EOF
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
  -D WITH_TLS=ON
cmake --build /src/build
mv /src/build/xmrig-proxy /xmrig-proxy
EOF

FROM --platform=$TARGET_PLATFORM cgr.dev/chainguard/static:latest AS runtime
COPY --from=build /xmrig-proxy /xmrig-proxy
ENTRYPOINT [ "/xmrig-proxy" ]
