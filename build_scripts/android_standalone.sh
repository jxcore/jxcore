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

SHOW_USAGE() {
  LOG $GREEN_COLOR "usage: android_compile_v8 <ndk_path> <arch>"
  LOG $GREEN_COLOR "arch: intel arm mips"
  exit
}

if [ $# -lt 2 ]
then
  LOG $RED_COLOR "no argument provided."
  SHOW_USAGE
fi

export ANDROID_NDK=$1
    
MAKE_INSTALL() {
  TARGET_DIR="out_android/sa_$1"

  mkdir -p $TARGET_DIR  
  mv $TARGET_DIR out
  ./configure --prefix=$TARGET_DIR/__/ --dest-os=android --dest-cpu=$1 --without-snapshot
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  
  make
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  
  mv out $TARGET_DIR
}

rm -rf out

OLD_PATH=$PATH
CPU_TYPE=0

if [ $2 == 'intel' ]
then
  export TOOLCHAIN=$PWD/android-toolchain-intel
  export PATH=$TOOLCHAIN/bin:$OLD_PATH
  export AR=i686-linux-android-ar
  export CC=i686-linux-android-gcc
  export CXX=i686-linux-android-g++
  export LINK=i686-linux-android-g++
  CPU_TYPE='ia32'
fi

if [ $2 == 'arm' ]
then
  export TOOLCHAIN=$PWD/android-toolchain-arm
  export PATH=$TOOLCHAIN/bin:$OLD_PATH
  export AR=arm-linux-androideabi-ar
  export CC=arm-linux-androideabi-gcc
  export CXX=arm-linux-androideabi-g++
  export LINK=arm-linux-androideabi-g++
  CPU_TYPE='arm'
fi

if [ $2 == 'mips' ]
then
  export TOOLCHAIN=$PWD/android-toolchain-mipsel
  export PATH=$TOOLCHAIN/bin:$OLD_PATH
  export AR=mipsel-linux-android-ar
  export CC=mipsel-linux-android-gcc
  export CXX=mipsel-linux-android-g++
  export LINK=mipsel-linux-android-g++
  CPU_TYPE='mipsel'
fi

if [ $CPU_TYPE == 0 ]
then
  LOG $RED_COLOR "Unknown architecture."
  SHOW_USAGE
fi

LOG $GREEN_COLOR "Compiling JXcore standalone for Android-$2\n"
MAKE_INSTALL $CPU_TYPE


LOG $GREEN_COLOR "jx binary is ready under out_android/sa_$CPU_TYPE/Release/\n"