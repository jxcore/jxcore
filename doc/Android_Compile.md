JXcore is ready for `Android OS`. On this tutorial, we will give you step-by-step instruction on how to compile JXcore for Android target.

First, you need `Android NDK`. You can download the latest NDK from [here](https://developer.android.com/tools/sdk/ndk/index.html)

Assuming you have already extracted `Android NDK` into `~/androidNDK` folder, next step is to use our prebuilt script file to create Android toolchains before the compilation process.

Before starting, please check the **prerequisites** from [this link](https://github.com/jxcore/jxcore/blob/master/doc/HOW_TO_COMPILE.md)

Go to root folder of jxcore project and;
```bash
$> build_scripts/android-configure.sh ~/androidNDK/
```
This should create `android-toolchain` and `android-toolchain-intel` folders under the project's root folder. We need this folders to prepare Android ARM and Intel binaries. 

Now we can compile jxcore for android. type;
```bash
$> build_scripts/android_compile.sh ~/androidNDK/
```

The previous script uses SpiderMonkey, if you want/need V8, please use android_compile_v8.sh instead.

This one should take a while. When it's finished, you can find the binaries and include files for ARM and INTEL platforms inside `out_android/android` folder.
