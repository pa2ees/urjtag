#!/bin/bash -ex

SUDO=${SUDO:-}

# for testing w/ docker container
[ "${SUDO}" == "sudo" ] || apt install -y autoconf autopoint libtool pkg-config

# ARM GNU compiler utilities
${SUDO} rm -f /etc/apt/sources.list /etc/apt/sources.list.d/* # eliminates 404 errors w/ apt update
cat .ci/xenial.list | ${SUDO} tee /etc/apt/sources.list.d/xenial.list
${SUDO} dpkg --add-architecture armhf
${SUDO} apt update || :
${SUDO} apt install -y gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf crossbuild-essential-armhf

# Python
PY_VER=3.5.2
wget -q https://www.python.org/ftp/python/${PY_VER}/Python-${PY_VER}.tgz && tar xf Python-${PY_VER}.tgz
pushd Python-${PY_VER}
echo ac_cv_file__dev_ptmx=no > CONFIG_SITE
echo ac_cv_file__dev_ptc=no >> CONFIG_SITE
CC=arm-linux-gnueabihf-gcc CPP=arm-linux-gnueabihf-cpp READELF=arm-linux-gnueabihf-readelf CONFIG_SITE=CONFIG_SITE \
./configure --host=arm-linux --build=armv7l --enable-shared --disable-ipv6 --exec-prefix=/usr/arm-linux-gnueabihf
make -j$(nproc)
${SUDO} make -j$(nproc) install
popd
${SUDO} rm -rf Python-${PY_VER}*
