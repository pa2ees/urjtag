#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
$wd/build-arm.sh arm64 aarch64-linux-gnu
