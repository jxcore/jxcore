JXcore is ready for `iOS` platform. On this tutorial, we will give you a step-by-step instruction on how to compile JXcore for iOS.

Before starting, please check the **prerequisites** from [this link](https://github.com/jxcore/jxcore/blob/master/doc/HOW_TO_COMPILE.md)

First, you need a `MAC!` and the latest `XCode` installed with iPhone device and simulator SDKs. 

Go to root folder of jxcore project and:
```bash
$> ./build_scripts/ios_compile.sh
```
Prepare a cup of coffee for yourself since this is going to take a while! When it's finished, 
you can find the 'fat' binaries and include files for ARM, ARM64, INTEL32, and INTEL64 targets under `out_ios/ios` folder

Coming Soon -> We are preparing the samples for mobile applications. 

What is next:  
[Embedding JXcore](https://github.com/jxcore/jxcore/blob/master/doc/native/Embedding_Basics.md)
