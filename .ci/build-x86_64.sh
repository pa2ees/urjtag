#!/bin/bash -ex

[ "${SUDO:-}" == "sudo" ] && travis=true || travis=false

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# docker container only
$travis || $wd/clean.sh

pushd urjtag

! $travis || sed --in-place -e '/GETTEXT_VERSION/s/0.19/0.18/' configure.ac

# `export` required for setup.py
export CC=gcc
PYTHON=/usr/bin/python3.5 ./autogen.sh --enable-stapl
make -j$(nproc)
! $travis || git checkout HEAD $(git rev-parse --show-toplevel)/urjtag/configure.ac
find . -name "*urjtag*.so*"

$wd/package.sh \
 amd64 \
 src/.libs/liburjtag.so.0 \
 bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so

popd # urjtag
