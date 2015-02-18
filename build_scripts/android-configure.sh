export TOOLCHAIN=$PWD/android-toolchain
rm -rf $TOOLCHAIN
mkdir -p $TOOLCHAIN
$1/build/tools/make-standalone-toolchain.sh \
    --toolchain=arm-linux-androideabi-4.8 \
    --arch=arm \
    --install-dir=$TOOLCHAIN \
    --platform=android-9
    
rm $TOOLCHAIN/bin/python

export TOOLCHAIN_INTEL=$PWD/android-toolchain-intel
rm -rf $TOOLCHAIN_INTEL
mkdir -p $TOOLCHAIN_INTEL
$1/build/tools/make-standalone-toolchain.sh \
    --toolchain=x86-4.8 \
    --arch=x86 \
    --install-dir=$TOOLCHAIN_INTEL \
    --platform=android-9
    
rm $TOOLCHAIN_INTEL/bin/python
    
echo android tools are copied. now call 'build_scripts/android-make.sh'