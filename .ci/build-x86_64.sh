#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

pushd urjtag

sed --in-place -e '/GETTEXT_VERSION/s/0.19/0.18/' configure.ac
PYTHON=/usr/bin/python3.5 ./autogen.sh
make -j$(nproc)
find . -name "*urjtag*.so*"

# package
pysyspath=/usr/lib/python3.5
mkdir -p dpkg/$pysyspath
cp -f src/.libs/liburjtag.so.0 dpkg/$pysyspath/
cp -f bindings/python/build/lib.linux-x86_64-3.5/urjtag.cpython-35m-x86_64-linux-gnu.so dpkg/$pysyspath/
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
 --architecture amd64 \
 --maintainer 'IMSAR FPGA Team <fpga@imsar.com>' \
 --description 'UrJTAG Python bindings' \
 --url 'https://www.imsar.com/' \
 --deb-no-default-config-files \
 --deb-systemd $wd/urjtag.service \
 --deb-systemd-restart-after-upgrade \
 .

popd # urjtag
