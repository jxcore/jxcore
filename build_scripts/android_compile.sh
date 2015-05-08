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
  LOG $GREEN_COLOR "usage: android_compile <ndk_path>\n"
  exit
fi

export ANDROID_NDK=$1

MIPS=out_android/mipsel
ARM7=out_android/arm
INTEL64=out_android/x64
INTEL32=out_android/ia32
FATBIN=out_android/android
    
MAKE_INSTALL() {
  TARGET_DIR="out_$1_droid"
  PREFIX_DIR="out_android/$1"
  mv $TARGET_DIR out
  ./configure --prefix=$PREFIX_DIR --static-library --dest-os=android --dest-cpu=$1 --engine-mozilla
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  rm -rf $PREFIX_DIR/bin
  make install
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  rm out/Release/*.a
  mv out $TARGET_DIR
  
  $STRIP -d $PREFIX_DIR/bin/libcares.a
  mv $PREFIX_DIR/bin/libcares.a "$PREFIX_DIR/bin/libcares_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libchrome_zlib.a
  mv $PREFIX_DIR/bin/libchrome_zlib.a "$PREFIX_DIR/bin/libchrome_zlib_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libhttp_parser.a
  mv $PREFIX_DIR/bin/libhttp_parser.a "$PREFIX_DIR/bin/libhttp_parser_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libjx.a
  mv $PREFIX_DIR/bin/libjx.a "$PREFIX_DIR/bin/libjx_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libmozjs.a
  mv $PREFIX_DIR/bin/libmozjs.a "$PREFIX_DIR/bin/libmozjs_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libopenssl.a
  mv $PREFIX_DIR/bin/libopenssl.a "$PREFIX_DIR/bin/libopenssl_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libuv.a
  mv $PREFIX_DIR/bin/libuv.a "$PREFIX_DIR/bin/libuv_$1.a"
  
  $STRIP -d $PREFIX_DIR/bin/libsqlite3.a
  mv $PREFIX_DIR/bin/libsqlite3.a "$PREFIX_DIR/bin/libsqlite3_$1.a"
}

COMBINE() {
  cp "$MIPS/bin/$1_mipsel.a" "$FATBIN/bin/"
  cp "$ARM7/bin/$1_arm.a" "$FATBIN/bin/"
  cp "$INTEL64/bin/$1_x64.a" "$FATBIN/bin/"
  cp "$INTEL32/bin/$1_ia32.a" "$FATBIN/bin/"
  ERROR_ABORT
}

mkdir out_mipsel_droid
mkdir out_arm_droid
mkdir out_x64_droid
mkdir out_ia32_droid
mkdir out_android

rm -rf out

OLD_PATH=$PATH
export TOOLCHAIN=$PWD/android-toolchain-mipsel
export PATH=$TOOLCHAIN/bin:$OLD_PATH
export AR=mipsel-linux-android-ar
export CC=mipsel-linux-android-gcc
export CXX=mipsel-linux-android-g++
export LINK=mipsel-linux-android-g++
export STRIP=mipsel-linux-android-strip

LOG $GREEN_COLOR "Compiling Android MIPS\n"
MAKE_INSTALL mipsel

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
mv $ARM7/include $FATBIN/

cp deps/mozjs/src/js.msg $FATBIN/include/node/

COMBINE "libcares"
COMBINE "libchrome_zlib"
COMBINE "libhttp_parser"
COMBINE "libjx"
COMBINE "libmozjs"
COMBINE "libopenssl"
COMBINE "libuv"
COMBINE "libsqlite3"

cp src/public/*.h $FATBIN/bin

rm -rf $MIPS
rm -rf $ARM7
rm -rf $INTEL32
rm -rf $INTEL64

LOG $GREEN_COLOR "JXcore Android binaries are ready under $FATBIN\n"
