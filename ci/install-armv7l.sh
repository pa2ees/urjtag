#!/bin/bash -ex

wd=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
$wd/install-arm.sh armhf arm-linux-gnueabihf armv7l
