#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
$wd/install-common.sh

if ! dpkg -l python3.5-dev; then
  ${SUDO:-} apt install -yq python3.5-dev
fi
