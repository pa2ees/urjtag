#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

pushd urjtag

# use `export` for setup.py
export CC=arm-linux-gnueabihf-gcc
export LDSHARED=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-cpp PYTHON=/usr/bin/python3.5 ./autogen.sh --host=arm-linux --build=armv7l
make -j$(nproc)
find . -name "*urjtag*.so*"

# package
pysyspath=/usr/lib/python3.5
mkdir -p dpkg/$pysyspath
cp -f src/.libs/liburjtag.so.0 dpkg/$pysyspath/
cp -f bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so \
 dpkg/$pysyspath/urjtag.cpython-35m-arm-linux-gnueabihf.so
fpm \
 --output-type deb \
 --input-type dir \
 --chdir dpkg \
 --force \
 --name urjtag \
 --version 0.0.0 \
 --iteration ${TRAVIS_BUILD_NUMBER:-0} \
 --license 'proprietary' \
 --vendor 'IMSAR LLC' \
 --architecture armhf \
 --maintainer 'IMSAR FPGA Team <fpga@imsar.com>' \
 --description 'UrJTAG Python bindings' \
 --url 'https://www.imsar.com/' \
 --deb-no-default-config-files \
 --deb-systemd $wd/urjtag.service \
 --deb-systemd-restart-after-upgrade \
 .

popd # urjtag
