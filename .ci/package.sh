#!/bin/bash -e

if [ $# -lt 2 ]; then
  echo "USAGE: $0 ARCH PYLIB_FILES"
  exit 1
fi
set -x

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

arch=$1
shift

pysyspath=/usr/lib/python3.5
mkdir -p dpkg/$pysyspath
cp -f $@ dpkg/$pysyspath/
profile=/etc/profile.d
mkdir -p dpkg/$profile
cp -f $wd/urjtag.sh dpkg/$profile

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
 --architecture $arch \
 --maintainer 'IMSAR FPGA Team <fpga@imsar.com>' \
 --description 'UrJTAG Python bindings' \
 --url 'https://www.imsar.com/' \
 --after-install $wd/urjtag.sh \
 --deb-no-default-config-files \
 .
