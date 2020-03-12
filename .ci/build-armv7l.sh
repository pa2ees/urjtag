#!/bin/bash -ex

pushd urjtag
# use `export` for setup.py
export CC=arm-linux-gnueabihf-gcc
export LDSHARED=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-cpp PYTHON=/usr/bin/python3.5 ./autogen.sh --host=arm-linux --build=armv7l
make -j$(nproc)
pushd ./bindings/python/build
ln -sf lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so urjtag.cpython-35m-arm-linux-gnueabihf.so
popd
find . -name "liburjtag.so*"
popd # urjtag
