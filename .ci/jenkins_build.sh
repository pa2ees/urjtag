#!/bin/bash -ex
gb=$(git rev-parse --show-toplevel)
[ -f ubuntu-ruby ] || wget -qN https://hub.docker.com/r/travisci/ubuntu-ruby
docker run -v $gb:$gb --rm -w $gb travisci/ubuntu-ruby:16.04 /bin/bash -c \
"apt install -y bison flex && \
MAJOR=0 MINOR=0 PATCH=1 BUILD_NUMBER=${BUILD_NUMBER} .ci/build-$build.sh && \
cp urjtag/urjtag_*.*.*-*_*.deb ."
