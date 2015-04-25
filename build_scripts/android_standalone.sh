
#!/bin/bash

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
    LOG $RED_COLOR "compilation aborted\n"
    exit  
  fi
}


ERROR_ABORT_MOVE() {
  if [[ $? != 0 ]]
  then
    $($1)
    LOG $RED_COLOR "compilation aborted for $2 target\n"
    exit  
  fi
}

if [ $# -eq 0 ]
then
  LOG $RED_COLOR "no argument provided."
  LOG $GREEN_COLOR "usage: android_compile_v8 <ndk_path>\n"
  exit
fi

export ANDROID_NDK=$1
    
MAKE_INSTALL() {
  PREFIX_DIR="out_android_$1"
  ./configure --prefix=$PREFIX_DIR --dest-os=android --dest-cpu=$1 --without-snapshot
  make
}

OLD_PATH=$PATH

export TOOLCHAIN=$PWD/android-toolchain-intel
export PATH=$TOOLCHAIN/bin:$OLD_PATH
export AR=i686-linux-android-ar
export CC=i686-linux-android-gcc
export CXX=i686-linux-android-g++
export LINK=i686-linux-android-g++

LOG $GREEN_COLOR "Compiling Android INTEL32\n"
MAKE_INSTALL ia32


LOG $GREEN_COLOR "JXcore Android binaries are ready\n"