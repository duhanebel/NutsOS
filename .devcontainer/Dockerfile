# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.205.2/containers/debian/.devcontainer/base.Dockerfile
# [Choice] Debian version (use bullseye or stretch on local arm64/Apple Silicon): bullseye, buster, stretch
ARG VARIANT="bullseye"
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}
#FROM debian:${VARIANT}

ENV PREFIX="/usr/local/"
ENV TARGET=i686-elf
ENV PATH="$PREFIX/bin:$PATH"

WORKDIR /tmp
RUN apt-get update && apt-get -y install \
    make                               \
    nasm                               \
    curl                               \
    build-essential                    \
    bison                              \
    flex                               \
    libgmp3-dev                        \
    libmpc-dev                         \
    libmpfr-dev                        \
    texinfo                            \
    libisl-dev                         \
    gdb-multiarch                      \
    lldb                               \
    qemu-system-i386                               
RUN curl https://ftp.nluug.nl/pub/gnu/binutils/binutils-2.37.tar.gz | tar xz \
    && curl https://mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-11.2.0/gcc-11.2.0.tar.gz | tar xz \
    && mkdir build-binutils \
    && mkdir build-gcc \
    && cd build-binutils \
    && ../binutils-2.37/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror \
    && make -j 8 && make install \
    && cd ../build-gcc \
    && ../gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers \
    && make -j 8 all-gcc && make -j 4 all-target-libgcc \
    && make install-gcc && make install-target-libgcc \
    && cd .. && rm -rf build-binutils build-gcc \
    && i686-elf-gcc --version
