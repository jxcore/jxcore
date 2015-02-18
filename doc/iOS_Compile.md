JXcore is ready for `iOS` On this tutorial, we will give you a step-by-step instruction on how to compile JXcore for iOS targets.

First, you need a `MAC!` and the latest `XCode` installed with iPhone device and simulator SDKs. 

Go to root folder of jxcore project and;
```bash
$> build_scripts/ios-compile.sh
```
Prepare a cup of coffee for yourself since this is going to take a while! When it's finished, 
you can find the 'fat' binaries and include files for ARM, ARM64 and INTEL64 platforms under the `/jxcoreIOS` folder. 
This folder contains the FAT binary files (ARM + ARM64 + INTEL64) If you need them separately, they are individually available 
from '/jxcoreIOSarm', '/jxcoreIOSarm64', and '/jxcoreIOSintel64' folders.


