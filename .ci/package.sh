#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

arch=$1
shift

pysyspath=/usr/lib/python3.5
mkdir -p dpkg/$pysyspath
cp -f $@ dpkg/$pysyspath/

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
 --deb-no-default-config-files \
 --deb-systemd $wd/urjtag.service \
 --deb-systemd-restart-after-upgrade \
 .
