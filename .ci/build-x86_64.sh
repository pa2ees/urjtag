#!/bin/bash -ex

pushd urjtag
sed --in-place -e '/GETTEXT_VERSION/s/0.19/0.18/' configure.ac
PYTHON=/usr/bin/python3.5 ./autogen.sh
make -j$(nproc)
popd
