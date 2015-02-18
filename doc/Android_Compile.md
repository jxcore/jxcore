JXcore is ready for `Android OS`. On this tutorial, we will give you step-by-step instruction on how to compile JXcore for Android target.

First, you need `Android NDK`. You can download the latest NDK from [here](https://developer.android.com/tools/sdk/ndk/index.html)

Assuming you have already extracted `Android NDK` into `~/androidNDK` folder, next step is to use our prebuilt script file to create Android toolchains before the compilation process.

Go to root folder of jxcore project and;
```bash
$> build_scripts/android-configure.sh ~/androidNDK/
```
This should create `android-toolchain` and `android-toolchain-intel` folders under the project's root folder. We need this folders to prepare Android ARM and Intel binaries. 

Now we can compile jxcore for android. type;
```bash
$> build_scripts/make-android.sh ~/androidNDK/
```

This one should take a while. When it's finished, you can find the binaries and include files for ARM and INTEL platforms inside `/jxcoreARM` and `/jxcoreIntel` folders.

Please keep in mind, these are 32 bit binaries. You can easily add 64 bit option by changing `--dest-cpu` parameter inside one of the `android-make-*` files. (i.e. for ARM64, use `arm64` . for Intel64, remove --dest-cpu)

Using `lipo` on unix, you could prepare your own fat binaries but for the sake of this document, it's not necessary.

**The prebuilt scripts uses V8 engine for Android platform. We are going to update them with SpiderMonkey JIT after SM ARM-JIT proxy stabilization.**
