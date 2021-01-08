#!/bin/bash -ex

[ "${SUDO:-}" == "sudo" ] && travis=true || travis=false

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

build=x86_64

# docker container only
$travis || $wd/clean.sh

pushd urjtag

# `export` required for setup.py
export CC=gcc
prefix=$wd/usr
PYTHON=/usr/bin/python3.5 \
 ./autogen.sh \
 --enable-svf \
 --enable-bsdl \
 --enable-stapl \
 --enable-relocatable \
 --prefix=$prefix
make -j$(nproc)
! $travis || git checkout HEAD $(git rev-parse --show-toplevel)/urjtag/configure.ac
find . -name "*urjtag*.so*" | xargs ls -l

make install

$wd/package.sh \
 amd64 \
 / \
 $prefix/bin/jtag \
 $prefix/share/urjtag \
 bindings/python/build/lib.linux-$build-3.5/urjtag.cpython-35m-$build-linux-gnu.so \
 src/.libs/liburjtag.so*

popd # urjtag
