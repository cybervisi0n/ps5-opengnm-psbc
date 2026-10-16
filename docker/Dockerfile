FROM debian:13

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential meson \
    python3 libnss-unknown fuse file \
    ninja-build wget ca-certificates git \
    clang unzip llvm lld zlib1g-dev \
    imagemagick cmake pkg-config python3 \
    python3-packaging python3-yaml python3-mako \
    flex bison python3-lxml python3-rnc2rng

RUN mkdir -p /home
RUN chmod 777 /home
