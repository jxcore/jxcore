export ANDROID_NDK=$1
export TOOLCHAIN_INTEL=$PWD/android-toolchain-intel
export PATH=$TOOLCHAIN_INTEL/bin:$PATH
export AR=i686-linux-android-ar
export CC=i686-linux-android-gcc
export CXX=i686-linux-android-g++
export LINK=i686-linux-android-g++

./configure \
--without-snapshot \
--dest-cpu=ia32 \
--dest-os=android \
--prefix=/jxcoreIntel \
--static-library

make install