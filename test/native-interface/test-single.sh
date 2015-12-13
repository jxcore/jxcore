#!/bin/bash

LIBRARY_PATH=$1

NORMAL_COLOR='\033[0m'
RED_COLOR='\033[0;31m'
GREEN_COLOR='\033[0;32m'
GREY_COLOR='\033[0;37m'

LOG() {
  COLOR="$1"
  TEXT="$2"
  echo -e "${COLOR}$TEXT ${NORMAL_COLOR}"
}

ERROR_ABORT() {
  if [[ $? != 0 ]]
  then
    LOG $RED_COLOR "$1\n"
    x=$(rm test)
    exit 1
  fi
}

if [ $# -gt 2 ]
then
  LOG $GREY_COLOR "Starting JXcore Native Interface Test"
else
  LOG $RED_COLOR "missing arguments."
  LOG $GREEN_COLOR "usage: test_native <lib_path> <test_folder> <v8 or sm>\n"
  exit 1
fi

LOG $GREY_COLOR "Compiling test $2"
ROOT=${PWD}
cd "$2"
if [[ $3 == "sm" ]]
then
  LOG $GREY_COLOR "SM"
  g++ -DJS_ENGINE_MOZJS -DJS_PUNBOX64 test-posix.cpp -stdlib=libstdc++ -lstdc++ -std=c++11 -O2 -I$LIBRARY_PATH/include/node \
    $LIBRARY_PATH/bin/libcares.a  $LIBRARY_PATH/bin/libjx.a $LIBRARY_PATH/bin/libsqlite3.a \
    $LIBRARY_PATH/bin/libchrome_zlib.a $LIBRARY_PATH/bin/libmozjs.a  $LIBRARY_PATH/bin/libuv.a \
    $LIBRARY_PATH/bin/libhttp_parser.a  $LIBRARY_PATH/bin/libopenssl.a -Wl -framework CoreServices \
    -Wno-c++11-compat-deprecated-writable-strings -Wno-deprecated-declarations -Wno-unknown-warning-option -o test
else
  if [[ $3 != "v8_328" ]]
  then
    LOG $GREY_COLOR "V8 3.14"
    g++ -DJS_ENGINE_V8 test-posix.cpp -stdlib=libstdc++ -lstdc++ -std=c++11 -O2 -I$LIBRARY_PATH/include/node \
      $LIBRARY_PATH/bin/libcares.a  $LIBRARY_PATH/bin/libjx.a $LIBRARY_PATH/bin/libsqlite3.a \
      $LIBRARY_PATH/bin/libchrome_zlib.a $LIBRARY_PATH/bin/libv8_base.a $LIBRARY_PATH/bin/libv8_nosnapshot.a  $LIBRARY_PATH/bin/libuv.a \
      $LIBRARY_PATH/bin/libhttp_parser.a  $LIBRARY_PATH/bin/libopenssl.a -Wl -framework CoreServices \
      -Wno-c++11-compat-deprecated-writable-strings -Wno-deprecated-declarations -Wno-unknown-warning-option -o test
  else
    LOG $GREY_COLOR "V8 3.28"
    g++ -DJS_ENGINE_V8 -DV8_IS_3_28 test-posix.cpp -stdlib=libstdc++ -lstdc++ -std=c++11 -O2 -I$LIBRARY_PATH/include/node \
      $LIBRARY_PATH/bin/libchrome_zlib.a $LIBRARY_PATH/bin/libv8_base.a $LIBRARY_PATH/bin/libv8_nosnapshot.a \
      $LIBRARY_PATH/bin/libuv.a $LIBRARY_PATH/bin/libcares.a  $LIBRARY_PATH/bin/libjx.a $LIBRARY_PATH/bin/libsqlite3.a \
      $LIBRARY_PATH/bin/libhttp_parser.a  $LIBRARY_PATH/bin/libopenssl.a -Wl -framework CoreServices \
      -Wno-c++11-compat-deprecated-writable-strings -Wno-deprecated-declarations -Wno-unknown-warning-option -o test
  fi
fi

ERROR_ABORT "compilation failed for the test '$2'"
ret=$(./test)
ERROR_ABORT "$ret\n'$2' test failed. $ret"

if [ $# -eq 3 ]
then
  rm test
fi

cd "$ROOT"

LOG $GREEN_COLOR "'$2' test passed"