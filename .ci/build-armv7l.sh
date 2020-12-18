#!/bin/bash -ex

[ "${SUDO:-}" == "sudo" ] && travis=true || travis=false

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

build=armv7l

# docker container only
$travis || $wd/clean.sh

pushd urjtag

# `export` required for setup.py
export CC=arm-linux-gnueabihf-gcc
export LDSHARED=arm-linux-gnueabihf-gcc
prefix=$wd/usr
CPP=arm-linux-gnueabihf-cpp PYTHON=/usr/bin/python3.5 \
 ./autogen.sh \
 --host=arm-linux-gnueabihf \
 --enable-svf \
 --enable-bsdl \
 --enable-stapl \
 --enable-relocatable --prefix=$prefix

# copy auto-generated files
cp src/bsdl/bsdl_bison.c.copy src/bsdl/bsdl_bison.c
cp src/bsdl/bsdl_bison.h.copy src/bsdl/bsdl_bison.h
cp src/bsdl/bsdl_flex.c.copy src/bsdl/bsdl_flex.c
cp src/bsdl/vhdl_bison.c.copy src/bsdl/vhdl_bison.c
cp src/bsdl/vhdl_bison.h.copy src/bsdl/vhdl_bison.h
cp src/bsdl/vhdl_flex.c.copy src/bsdl/vhdl_flex.c
cp src/svf/svf_bison.c.copy src/svf/svf_bison.c
cp src/svf/svf_bison.h.copy src/svf/svf_bison.h
cp src/svf/svf_flex.c.copy src/svf/svf_flex.c

make -j$(nproc)
find . -name "*urjtag*.so*" | xargs ls -l

make install

cp -f \
 bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so \
 urjtag.cpython-35m-arm-linux-gnueabihf.so
$wd/package.sh \
 armhf \
 $prefix/bin/jtag \
 $prefix/share/urjtag \
 urjtag.cpython-35m-arm-linux-gnueabihf.so \
 src/.libs/liburjtag.so*

popd # urjtag
