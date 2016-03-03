# To run the tests:

## Unix/Macintosh:

    make test

The command above executes basic tests from *simple* and *message* subfolders of *test* directory.

However, you can also run tests for specified folders, like:

    make test test-simple test-message

Other category of tests resides in *jxcore* subfolder. They can be run as plain *.js* files, 
packaged or native packaged. Extra flags can be provided as parameters 
- see [test/README.md](../test/README.md) for more details.

The following command is for testing *jxcore* tests as plain .js files:

    make test test-jxcore

or

    make test test-jxcore flags=j

As packages:

    make test test-jxcore flags=p

As native packages:

    make test test-jxcore flags=n

All at once:

    make test test-jxcore flags=a

## Windows:

    vcbuild.bat test

## Non-python platforms

On some platforms, where python is not installed/available (e.g. android) the tests can be launched by 
JXcore itself with the following command:

    jx test/test.js jxcore

### Android standalone binaries

The steps below can be useful for testing standalone jx binaries on android with script mentioned above.
Basically it is about copying the test folder and standalone jx to the device and then running the 
test through `adb shell` command.

Sample preparation script:

```bash
#!/bin/bash

# working folder on android device:
DEST=/data/local/tmp/jxcore
# path to adb tool:
ADB=~/android-sdks/platform-tools/adb
# path to local JXcore git repository:
JXREPO=~/Documents/GitHub/jxcore

# copy test folder from jxcore local repository
$ADB shell mkdir -p $DEST
$ADB shell rm -r $DEST/test/
$ADB push -p $JXREPO/test $DEST/test/

# compile standalone binary for android, if needed
# cd $JXREPO
# sudo ./build_scripts/android_standalone.sh ~/android-ndk-r10d arm v8

# copy jx to the device
$ADB push -p $JXREPO/out_android/sa_arm_v8/Release/jx $DEST
```

Now you're ready to run the tests:

```bash
$ ~/android-sdks/platform-tools/adb shell
shell@android:/ $ cd /data/local/tmp/jxcore
shell@android:/data/local/tmp/jxcore $ ./jx test/test.js jxcore
```
    
## Native Interface

Currently native interface tests are *nix only. (We would appreciate if somebody would add Windows 
support for the tests. This is indeed not necessary. JXcore native interface 'jx-ni' works also 
for Windows applications.)

In order to run JX-ni tests you need JXcore is installed on your system. Either compile from the 
sources or download it from [jxcore/jxcore-release](https://github.com/jxcore/jxcore-release)

Assuming you are under the root folder of the project; first you should compile the project as a 
static library. Let's say you want to do it for SpiderMonkey build. 
```
> sudo ./configure --prefix=/testBin --engine-mozilla --static-library
> sudo make install
```

Now you can test the native interface;
```
> cd tests/native-interface
> ./run-tests.js /testBin sm 50
```

Number 50 at the end of the second command line corresponds to the number of runs per each test case. 
It's a bad but helpful hack! For your own sake, you may put 1 instead. If you are planning to contribute 
on native interface please make sure the test cases are passing 50 runs for both sm and v8.



