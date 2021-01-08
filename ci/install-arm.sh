#!/bin/bash -ex

if [ $# -lt 3 ]; then
  echo "USAGE: $0 ARCH TOOLCHAIN MACHINE"
  exit 1
fi
arch=$1
toolchain=$2
machine=$3

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
$wd/install-common.sh

SUDO=${SUDO:-}

# ARM GNU compiler utilities
if [ ! -f /usr/bin/$toolchain-gcc ]; then
  ${SUDO} rm -f /etc/apt/sources.list /etc/apt/sources.list.d/* # eliminates 404 errors w/ apt update
  cat ci/xenial.list | ${SUDO} tee /etc/apt/sources.list.d/xenial.list
  ${SUDO} dpkg --add-architecture $arch
  ${SUDO} apt update || :
  ${SUDO} apt install -yq gcc-$toolchain crossbuild-essential-$arch python3-dev
fi

# Python
if [ ! -f /usr/local/include/python3.5m/Python.h ]; then
  PY_VER=3.5.2
  ${SUDO} apt install -yq wget
  wget -qN https://www.python.org/ftp/python/${PY_VER}/Python-${PY_VER}.tgz
  tar xf Python-${PY_VER}.tgz
  pushd Python-${PY_VER}
  echo ac_cv_file__dev_ptmx=no > CONFIG_SITE
  echo ac_cv_file__dev_ptc=no >> CONFIG_SITE
  CONFIG_SITE=CONFIG_SITE \
   CC=$toolchain-gcc CPP=$toolchain-cpp \
   READELF=$toolchain-readelf \
   ./configure \
   --host=$toolchain \
   --build=$machine \
   --enable-shared \
   --disable-ipv6 \
   --exec-prefix=/usr/$toolchain || cat config.log
  make -j$(nproc)
  ${SUDO} make -j$(nproc) install
  popd
  ${SUDO} rm -rf Python-${PY_VER}*
fi

#dpkg -l gawk || ${SUDO} apt install -y gawk
