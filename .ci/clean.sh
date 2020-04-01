#!/bin/bash -ex
gb=$(git rev-parse --show-toplevel)
rm -rf \
 urjtag/bindings/python/setup.py \
 urjtag/dpkg \
 urjtag/urjtag.cpython*.so \
 urjtag/urjtag*.deb
[ ! -f $gb/urjtag/Makefile ] || make -C $gb/urjtag distclean || :
