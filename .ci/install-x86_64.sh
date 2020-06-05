#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
$wd/install-common.sh

dpkg -l python3.5-dev || ${SUDO:-} apt install -yq python3.5-dev
