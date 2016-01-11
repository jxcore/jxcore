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
  LOG $GREEN_COLOR "usage: android_compile <ndk_path> <optionally --embed-leveldown>\n"
  exit
fi

export ANDROID_NDK=$1

CONF_EXTRAS=
MIPS=0 #out_mipsel_droid
ARM64=0 #out_arm64_droid

if [ $# -eq 2 ]
then
  CONF_EXTRAS=$2
fi

ARM7=out_arm_droid
INTEL64=out_x64_droid
INTEL32=out_ia32_droid
FATBIN=out_android/android
    
MAKE_INSTALL() {
  TARGET_DIR="out_$1_droid"
  mv $TARGET_DIR out
  ./configure --static-library --dest-os=android --dest-cpu=$1 --engine-mozilla --compress-internals $CONF_EXTRAS
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  make -j 2
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  
  PREFIX_DIR="out/Release"
  $STRIP -d $PREFIX_DIR/libcares.a
  mv $PREFIX_DIR/libcares.a "$PREFIX_DIR/libcares_$1.a"
  
  $STRIP -d $PREFIX_DIR/libchrome_zlib.a
  mv $PREFIX_DIR/libchrome_zlib.a "$PREFIX_DIR/libchrome_zlib_$1.a"
  
  $STRIP -d $PREFIX_DIR/libhttp_parser.a
  mv $PREFIX_DIR/libhttp_parser.a "$PREFIX_DIR/libhttp_parser_$1.a"
  
  $STRIP -d $PREFIX_DIR/libjx.a
  mv $PREFIX_DIR/libjx.a "$PREFIX_DIR/libjx_$1.a"
  
  $STRIP -d $PREFIX_DIR/libmozjs.a
  mv $PREFIX_DIR/libmozjs.a "$PREFIX_DIR/libmozjs_$1.a"
  
  $STRIP -d $PREFIX_DIR/libopenssl.a
  mv $PREFIX_DIR/libopenssl.a "$PREFIX_DIR/libopenssl_$1.a"
  
  $STRIP -d $PREFIX_DIR/libuv.a
  mv $PREFIX_DIR/libuv.a "$PREFIX_DIR/libuv_$1.a"
  
  $STRIP -d $PREFIX_DIR/libsqlite3.a
  mv $PREFIX_DIR/libsqlite3.a "$PREFIX_DIR/libsqlite3_$1.a"
  
if [ "$CONF_EXTRAS" == "--embed-leveldown" ]
then
  $STRIP -d $PREFIX_DIR/libleveldown.a
  mv $PREFIX_DIR/libleveldown.a "$PREFIX_DIR/libleveldown_$1.a"
  
  $STRIP -d $PREFIX_DIR/libsnappy.a
  mv $PREFIX_DIR/libsnappy.a "$PREFIX_DIR/libsnappy_$1.a"
  
  $STRIP -d $PREFIX_DIR/libleveldb.a
  mv $PREFIX_DIR/libleveldb.a "$PREFIX_DIR/libleveldb_$1.a"
fi
  
  mv out $TARGET_DIR
}

COMBINE() {
if [ $MIPS != 0 ]
then
  mv "$MIPS/Release/$1_mipsel.a" "$FATBIN/bin/"
fi
if [ $ARM64 != 0 ]
then
  mv "$ARM64/Release/$1_arm64.a" "$FATBIN/bin/"
fi
  mv "$ARM7/Release/$1_arm.a" "$FATBIN/bin/"
  mv "$INTEL64/Release/$1_x64.a" "$FATBIN/bin/"
  mv "$INTEL32/Release/$1_ia32.a" "$FATBIN/bin/"
  ERROR_ABORT
}

if [ $MIPS != 0 ]
then
  mkdir out_mipsel_droid
fi

if [ $ARM64 != 0 ]
then
  mkdir out_arm64_droid
fi

mkdir out_arm_droid
mkdir out_x64_droid
mkdir out_ia32_droid
mkdir out_android

rm -rf out

OLD_PATH=$PATH
if [ $MIPS != 0 ]
then
  export TOOLCHAIN=$PWD/android-toolchain-mipsel
  export PATH=$TOOLCHAIN/bin:$OLD_PATH
  export AR=mipsel-linux-android-ar
  export CC=mipsel-linux-android-gcc
  export CXX=mipsel-linux-android-g++
  export LINK=mipsel-linux-android-g++
  export STRIP=mipsel-linux-android-strip

  LOG $GREEN_COLOR "Compiling Android MIPS\n"
  MAKE_INSTALL mipsel
fi

if [ $ARM64 != 0 ]
then
  export TOOLCHAIN=$PWD/android-toolchain-arm64
  export PATH=$TOOLCHAIN/bin:$OLD_PATH
  export AR=aarch64-linux-android-ar
  export CC=aarch64-linux-android-gcc
  export CXX=aarch64-linux-android-g++
  export LINK=aarch64-linux-android-g++
  export STRIP=aarch64-linux-android-strip

  LOG $GREEN_COLOR "Compiling Android ARM64\n"
  MAKE_INSTALL arm64
fi

export TOOLCHAIN=$PWD/android-toolchain-arm
export PATH=$TOOLCHAIN/bin:$OLD_PATH
export AR=arm-linux-androideabi-ar
export CC=arm-linux-androideabi-gcc
export CXX=arm-linux-androideabi-g++
export LINK=arm-linux-androideabi-g++
export STRIP=arm-linux-androideabi-strip

LOG $GREEN_COLOR "Compiling Android ARM7\n"
MAKE_INSTALL arm

export TOOLCHAIN=$PWD/android-toolchain-intelx64
export PATH=$TOOLCHAIN/bin:$OLD_PATH
export AR=x86_64-linux-android-ar
export CC=x86_64-linux-android-gcc
export CXX=x86_64-linux-android-g++
export LINK=x86_64-linux-android-g++
export STRIP=x86_64-linux-android-strip

LOG $GREEN_COLOR "Compiling Android INTEL64\n"
MAKE_INSTALL x64

export TOOLCHAIN=$PWD/android-toolchain-intel
export PATH=$TOOLCHAIN/bin:$OLD_PATH
export AR=i686-linux-android-ar
export CC=i686-linux-android-gcc
export CXX=i686-linux-android-g++
export LINK=i686-linux-android-g++
export STRIP=i686-linux-android-strip

LOG $GREEN_COLOR "Compiling Android INTEL32\n"
MAKE_INSTALL ia32

LOG $GREEN_COLOR "Preparing FAT binaries\n"
rm -rf $FATBIN
mkdir -p $FATBIN/bin

COMBINE "libcares"
COMBINE "libchrome_zlib"
COMBINE "libhttp_parser"
COMBINE "libjx"
COMBINE "libmozjs"
COMBINE "libopenssl"
COMBINE "libuv"
COMBINE "libsqlite3"

if [ "$CONF_EXTRAS" == "--embed-leveldown" ]
then
  COMBINE "libleveldown"
  COMBINE "libsnappy"
  COMBINE "libleveldb"
fi

cp src/public/*.h $FATBIN/bin

LOG $GREEN_COLOR "JXcore Android binaries are ready under $FATBIN\n"
