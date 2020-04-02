#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
$wd/install-common.sh

SUDO=${SUDO:-}

# ARM GNU compiler utilities
if [ ! -f /usr/bin/arm-linux-gnueabihf-gcc ]; then
  ${SUDO} rm -f /etc/apt/sources.list /etc/apt/sources.list.d/* # eliminates 404 errors w/ apt update
  cat .ci/xenial.list | ${SUDO} tee /etc/apt/sources.list.d/xenial.list
  ${SUDO} dpkg --add-architecture armhf
  ${SUDO} apt update || :
  ${SUDO} apt install -yq gcc-arm-linux-gnueabihf crossbuild-essential-armhf
fi

# Python
if [ ! -f /usr/local/include/python3.5m/Python.h ]; then
  PY_VER=3.5.2
  apt install -yq wget
  wget -qN https://www.python.org/ftp/python/${PY_VER}/Python-${PY_VER}.tgz && tar xf Python-${PY_VER}.tgz
  pushd Python-${PY_VER}
  echo ac_cv_file__dev_ptmx=no > CONFIG_SITE
  echo ac_cv_file__dev_ptc=no >> CONFIG_SITE
  CC=arm-linux-gnueabihf-gcc CPP=arm-linux-gnueabihf-cpp READELF=arm-linux-gnueabihf-readelf CONFIG_SITE=CONFIG_SITE \
  ./configure --host=arm-linux --build=armv7l --enable-shared --disable-ipv6 --exec-prefix=/usr/arm-linux-gnueabihf
  make -j$(nproc)
  ${SUDO} make -j$(nproc) install
  popd
  ${SUDO} rm -rf Python-${PY_VER}*
fi
