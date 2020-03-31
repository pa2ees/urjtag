#!/bin/bash -e

if [ $# -lt 2 ]; then
  echo "USAGE: $0 ARCH JTAG PYLIB_FILES"
  exit 1
fi
set -x

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

arch=$1
shift
jtag=$1
shift

pysyspath=/usr/lib/python3.5
mkdir -p dpkg/$pysyspath
cp -f $@ dpkg/$pysyspath/
profile=/etc/profile.d
mkdir -p dpkg/$profile
cp -f $wd/urjtag.sh dpkg/$profile
mkdir -p dpkg/usr/bin
cp -f $jtag dpkg/usr/bin/

# create after-install script
cat <<EOF > after-install.sh
cd $pysyspath
ln -sf liburjtag.so.0 liburjtag.so
EOF
cat $wd/urjtag.sh >> after-install.sh

# create before-remove script
cat <<EOF > before-remove.sh
rm -f $pysyspath/liburjtag.so
EOF

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
 --after-install after-install.sh \
 --before-remove before-remove.sh \
 --deb-no-default-config-files \
 .
