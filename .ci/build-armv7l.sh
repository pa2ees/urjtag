#!/bin/bash -ex

[ "${SUDO:-}" == "sudo" ] && travis=true || travis=false

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# docker container only
$travis || $wd/clean.sh

pushd urjtag

# `export` required for setup.py
export CC=arm-linux-gnueabihf-gcc
export LDSHARED=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-cpp PYTHON=/usr/bin/python3.5 LDFLAGS="-L/usr/lib/python3.5" \
 ./autogen.sh --host=arm-linux --build=armv7l --enable-stapl \
 --enable-relocatable --bindir=/usr/bin --prefix=/nonexistent
make -j$(nproc)
find . -name "*urjtag*.so*"

make -C src/apps/jtag install
cp /usr/bin/jtag src/apps/jtag/jtag-relocatable

cp -f \
 bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so \
 urjtag.cpython-35m-arm-linux-gnueabihf.so
$wd/package.sh \
 armhf \
 /usr/bin/jtag \
 src/.libs/liburjtag.so.0 \
 urjtag.cpython-35m-arm-linux-gnueabihf.so

popd # urjtag
