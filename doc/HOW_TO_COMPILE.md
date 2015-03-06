Prerequisites (Unix only):

    * GCC 4.2 or newer (for SpiderMonkey builds 4.7+)
    * Python 2.6 or 2.7
    * GNU Make 3.81 or newer
    * libexecinfo (FreeBSD and OpenBSD only)
    * for SpiderMonkey : 'which' python module (sudo easy_install tools/which-1.1.0-py2.7.egg)
    * for Windows (VS2012+) 

First clone the source codes from Github: (or Download as a .zip file)
```bash
git clone https://github.com/jxcore/jxcore.git
```

You may want to jump into [Android](Android_Compile.md), or [iOS](iOS_Compile.md) compilation details.

To compile for desktop/server environments; 

**SpiderMonkey**
```bash
./configure --prefix=/jxcoreSM --engine-mozilla
make install
```

**V8**
```bash
./configure --prefix=/jxcoreV8
make install
```

On Windows;

```
/ $> vcbuild.bat
```

After a successful compilation process, you should have jxcore installed into `/jxcoreSM/bin` or `/jxcoreV8/bin` folder (depending on the engine selection). 

JXcore internal JavaScript files can be embedded in two ways (compressed, or as-is). If you are planning to use `jx` binary for a native package creation, 
we advice you to use the `compressed` build. !! In order to compile with `compressed` build, you should have a `jx` binary ready on your platform. You may 
compile the non-compressed version first and then `compressed` one second.

**SpiderMonkey and Compressed Internals**
```
./configure --prefix=/jxcoreSM --engine-mozilla --compress-internals
make install
```

**V8 and Compressed Internals**
```bash
./configure --prefix=/jxcoreV8 --compress-internals
make install
```

Windows;
```
/ $> vcbuild.bat --compress-internals
```

**Compile as a Static Library** 
You can compile JXcore as a `static library` and embed it into your solution.

Simply add `--static-library` parameter to one of the above `configure` definitions. You should have the compiled lib files inside the target installation folder. 

On Windows;
```
/ $> vcbuild.bat --shared-library
```

**Notes**

If your python binary is in a non-standard location or has a
non-standard name, run the following instead:

    export PYTHON=/path/to/python
    $PYTHON ./configure
    make
    make install

You can download (latest stable) binaries for various operating systems from  
[http://jxcore.com/downloads/](http://jxcore.com/downloads/)  
(Future `jxcore.io` releases will be shared from `http://jxcore.io`)
