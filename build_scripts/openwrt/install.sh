#!/bin/bash

# Copyright & License details are available under JXCORE_LICENSE file

NORMAL_COLOR='\033[0m'
RED_COLOR='\033[0;31m'
GREEN_COLOR='\033[0;32m'
GRAY_COLOR='\033[0;37m'

LOG() {
  COLOR="$1"
  TEXT="$2"
  echo -e "${COLOR}$TEXT ${NORMAL_COLOR}"
}


ERROR_ABORT() {
  if [[ $? != 0 ]]
  then
    LOG $RED_COLOR "Installation process aborted\n"
    exit  
  fi
}

echo "OpenWrt JXcore package creator for Ubuntu\n"

LOG $GREEN_COLOR "Installing wget"
sudo apt-get install -y wget
ERROR_ABORT

LOG $GREEN_COLOR "Creating wrt-jx folder"
$(mkdir wrt-jx)

cd wrt-jx

wget https://raw.githubusercontent.com/jxcore/jxcore/master/build_scripts/openwrt/create_package.js
ERROR_ABORT

chmod +x create_package.js

wget https://raw.githubusercontent.com/jxcore/jxcore/master/build_scripts/openwrt/Makefile
ERROR_ABORT

curl http://jxcore.com/xil.sh | sudo bash -s force v8
ERROR_ABORT

LOG $GREEN_COLOR "Installing System Dependencies"
sudo apt-get update -y
sudo apt-get install -y git-core libssl-dev libncurses5-dev unzip \
 subversion mercurial gawk git build-essential \
 ccache  curl wget

LOG $GREEN_COLOR "\nUse 'wrt-jx/create_package.js <link to OpenWrt SDK> <arm or mipsel>'"
