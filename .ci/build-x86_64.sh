#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

pushd urjtag

sed --in-place -e '/GETTEXT_VERSION/s/0.19/0.18/' configure.ac
PYTHON=/usr/bin/python3.5 ./autogen.sh --enable-stapl
make -j$(nproc)
find . -name "*urjtag*.so*"

$wd/package.sh \
 amd64 \
 src/.libs/liburjtag.so.0 \
 bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so

popd # urjtag
