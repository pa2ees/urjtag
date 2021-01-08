#!/bin/bash -ex

if [ $# -lt 2 ]; then
  echo "USAGE: $0 ARCH TOOLCHAIN"
  exit 1
fi
arch=$1
toolchain=$2

[ "${SUDO:-}" == "sudo" ] && travis=true || travis=false

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# docker container only
$travis || $wd/clean.sh

pushd urjtag

# `export` required for setup.py
export CC=$toolchain-gcc
export LDSHARED=$toolchain-gcc
prefix=$wd/usr
CPP=$toolchain-cpp \
 PYTHON=/usr/bin/python3.5 \
 ./autogen.sh \
 --host=$toolchain \
 --enable-svf \
 --enable-bsdl \
 --enable-stapl \
 --enable-relocatable \
 --prefix=$prefix

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
 urjtag.cpython-35m-$toolchain.so
$wd/package.sh \
 $arch \
 $toolchain \
 $prefix/bin/jtag \
 $prefix/share/urjtag \
 urjtag.cpython-35m-$toolchain.so \
 src/.libs/liburjtag.so*

popd # urjtag
