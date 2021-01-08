#!/bin/bash -e

if [ $# -lt 6 ]; then
  echo "USAGE: $0 ARCH TOOLCHAIN BIN SHARE PYLIB SHARED_OBJS"
  exit 1
fi
set -x

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

arch=$1
shift

toolchain=$1
shift

# binary
mkdir -p dpkg/usr/bin
cp -f $1 dpkg/usr/bin/
shift

# share
mkdir -p dpkg/usr/share
cp -rf $1 dpkg/usr/share/
shift

# urjtag.cpython-*.so
lib=/usr/lib/python3.5
mkdir -p dpkg/$lib
cp -f $1 dpkg/$lib/
shift

# liburjtag.so*
lib=/usr/lib/$toolchain
mkdir -p dpkg/$lib
cp -fP $@ dpkg/$lib/

find dpkg \( -type f -o -type l \) | xargs ls -l

MAJOR=${MAJOR:-0}
MINOR=${MINOR:-0}
PATCH=${PATCH:-0}

fpm \
 --output-type deb \
 --input-type dir \
 --chdir dpkg \
 --force \
 --name imsar-urjtag \
 --version ${MAJOR}.${MINOR}.${PATCH} \
 --iteration ${BUILD_NUMBER:-0} \
 --license 'GPL' \
 --vendor 'IMSAR LLC' \
 --replaces urjtag \
 --architecture $arch \
 --maintainer 'IMSAR FPGA Team <fpga@imsar.com>' \
 --description 'UrJTAG Python bindings' \
 --url 'https://www.imsar.com/' \
 --deb-no-default-config-files \
 .
