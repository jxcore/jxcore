#!/bin/bash

NORMAL_COLOR='\033[0m'
RED_COLOR='\033[0;31m'
GREEN_COLOR='\033[0;32m'
GRAY_COLOR='\033[0;37m'

INTEL64=out_osx/x64
INTEL32=out_osx/ia32
FATBIN=out_osx/osx

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

NO_STRIP=0
if [[ $1 == "--no-strip" ]]
then
  NO_STRIP=1
fi

CONF_EXTRAS=

if [ $# -eq 1 ]
then
  CONF_EXTRAS=$1
fi


MAKE_INSTALL() {
  TARGET_DIR="out_$1_osx"
  PREFIX_DIR="out_osx/$1"
  mv $TARGET_DIR out
  ./configure --prefix=$PREFIX_DIR --static-library --dest-cpu=$1 --engine-mozilla $CONF_EXTRAS
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  rm -rf $PREFIX_DIR/bin
  make -j 2 install
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  mv out $TARGET_DIR
	
  mv $PREFIX_DIR/bin/libcares.a "$PREFIX_DIR/bin/libcares_$1.a"
  mv $PREFIX_DIR/bin/libchrome_zlib.a "$PREFIX_DIR/bin/libchrome_zlib_$1.a"
  mv $PREFIX_DIR/bin/libhttp_parser.a "$PREFIX_DIR/bin/libhttp_parser_$1.a"
  mv $PREFIX_DIR/bin/libjx.a "$PREFIX_DIR/bin/libjx_$1.a"
  mv $PREFIX_DIR/bin/libmozjs.a "$PREFIX_DIR/bin/libmozjs_$1.a"
  mv $PREFIX_DIR/bin/libopenssl.a "$PREFIX_DIR/bin/libopenssl_$1.a"
  mv $PREFIX_DIR/bin/libuv.a "$PREFIX_DIR/bin/libuv_$1.a"
  mv $PREFIX_DIR/bin/libsqlite3.a "$PREFIX_DIR/bin/libsqlite3_$1.a"
  
  if [ "$CONF_EXTRAS" == "--embed-leveldown" ]
  then
    mv $TARGET_DIR/Release/libleveldown.a "$PREFIX_DIR/bin/libleveldown_$1.a"
    mv $TARGET_DIR/Release/libsnappy.a "$PREFIX_DIR/bin/libsnappy_$1.a"
    mv $TARGET_DIR/Release/libleveldb.a "$PREFIX_DIR/bin/libleveldb_$1.a"
  fi
  rm $TARGET_DIR/Release/*.a
}


MAKE_FAT() {
  if [[ $NO_STRIP == 0 ]]
  then
    strip -x "$INTEL64/bin/$1_x64.a"
    strip -x "$INTEL32/bin/$1_ia32.a"
  fi
  
  lipo -create "$INTEL64/bin/$1_x64.a" "$INTEL32/bin/$1_ia32.a" -output "$FATBIN/bin/$1.a"
  ERROR_ABORT
}


mkdir out_x64_osx
mkdir out_ia32_osx
mkdir out_osx

rm -rf out

LOG $GREEN_COLOR "Compiling OSX INTEL32\n"
MAKE_INSTALL ia32

LOG $GREEN_COLOR "Compiling OSX INTEL64\n"
MAKE_INSTALL x64
 

LOG $GREEN_COLOR "Preparing FAT binaries\n"
rm -rf $FATBIN
mkdir -p $FATBIN/bin
mv $ARM7/include $FATBIN/

cp deps/mozjs/src/js.msg $FATBIN/include/node/

MAKE_FAT "libcares"
MAKE_FAT "libchrome_zlib"
MAKE_FAT "libhttp_parser"
MAKE_FAT "libjx"
MAKE_FAT "libmozjs"
MAKE_FAT "libopenssl"
MAKE_FAT "libuv"
MAKE_FAT "libsqlite3"
if [ "$CONF_EXTRAS" == "--embed-leveldown" ]
then
  MAKE_FAT "libleveldown"
  MAKE_FAT "libleveldb"
  MAKE_FAT "libsnappy"
fi

cp src/public/*.h $FATBIN/bin

rm -rf $INTEL32
rm -rf $INTEL64

LOG $GREEN_COLOR "JXcore OSX binaries are ready under $FATBIN"
