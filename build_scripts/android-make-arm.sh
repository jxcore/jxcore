export ANDROID_NDK=$1
export TOOLCHAIN=$PWD/android-toolchain
export PATH=$TOOLCHAIN/bin:$PATH
export AR=arm-linux-androideabi-ar
export CC=arm-linux-androideabi-gcc
export CXX=arm-linux-androideabi-g++
export LINK=arm-linux-androideabi-g++

./configure \
    --without-snapshot \
    --dest-cpu=arm \
    --dest-os=android \
    --prefix=/jxcoreARM \
    --static-library

make install