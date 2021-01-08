#!/bin/bash -e

[ "${SUDO:-}" == "sudo" ] && travis=true || travis=false

# docker container only
$travis || [ -f /usr/bin/autoreconf ] || apt install -yq autoconf autopoint libtool pkg-config
$travis || [ -f /usr/bin/flex ] || ${SUDO} apt install -y flex
$travis || [ -f /usr/bin/bison ] || ${SUDO} apt install -y bison
