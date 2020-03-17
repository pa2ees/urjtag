#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

pushd urjtag

# use `export` for setup.py
export CC=arm-linux-gnueabihf-gcc
export LDSHARED=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-cpp PYTHON=/usr/bin/python3.5 \
 ./autogen.sh --host=arm-linux --build=armv7l --enable-stapl
make -j$(nproc)
find . -name "*urjtag*.so*"

cp -f \
 bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so \
 urjtag.cpython-35m-arm-linux-gnueabihf.so
$wd/package.sh \
 armhf \
 src/.libs/liburjtag.so.0 \
 urjtag.cpython-35m-arm-linux-gnueabihf.so

popd # urjtag
