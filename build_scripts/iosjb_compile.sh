#!/bin/bash

NORMAL_COLOR='\033[0m'
RED_COLOR='\033[0;31m'
GREEN_COLOR='\033[0;32m'
GRAY_COLOR='\033[0;37m'

ARM7=out_ios/arm
ARM7s=out_ios/armv7s
ARM64=out_ios/arm64
FATBIN=out_ios/ios

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


MAKE_INSTALL() {
  TARGET_DIR="out_$1_ios"
  PREFIX_DIR="out_ios/$1"
  mv $TARGET_DIR out
  ./configure --prefix=$PREFIX_DIR --dest-os=ios --dest-cpu=$1 --engine-mozilla
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  rm -rf $PREFIX_DIR/bin
  make install -j8
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  mv out $TARGET_DIR
}


MAKE_FAT() {
	lipo -create "$ARM64/bin/$1" "$ARM7/bin/$1" "$ARM7s/bin/$1" -output "$FATBIN/bin/$1"
	ERROR_ABORT
}


mkdir out_armv7s_ios
mkdir out_arm_ios
mkdir out_arm64_ios
mkdir out_ios

rm -rf out

LOG $GREEN_COLOR "Compiling IOS ARMv7\n"
MAKE_INSTALL arm

LOG $GREEN_COLOR "Compiling IOS ARMv7s\n"
MAKE_INSTALL armv7s

LOG $GREEN_COLOR "Compiling IOS ARM64\n"
MAKE_INSTALL arm64
 

LOG $GREEN_COLOR "Preparing FAT binaries\n"
rm -rf $FATBIN
mkdir -p $FATBIN/bin
mv $ARM7/include $FATBIN/

cp deps/mozjs/src/js.msg $FATBIN/include/node/

MAKE_FAT "jx"

cp src/public/*.h $FATBIN/bin

rm -rf $ARM7s
rm -rf $ARM7
rm -rf $ARM64

LOG $GREEN_COLOR "JXcore iOS binaries are ready under $FATBIN"
