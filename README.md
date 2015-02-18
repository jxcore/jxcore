### Evented I/O for (SpiderMonkey, V8, and..) JavaScript

JXcore extends Node.JS™ with additional features for developing mobile and embedded applications using JavaScript and leveraging the Node ecosystem.

### Features include

  - Support for core Node.JS features
  - Embeddable Interface
  - Publish to Mobile Platforms (Android, iOS ..)
  - Supports Multiple JavaScript Engines
  - Multi-threading Capabilities
  - Process Configuration & Monitor
  - In-memory File System
  - Application Packaging
  - Support for the latest JavaScript features (ES6, ASM.JS ..)

### To build:

Prerequisites (Unix only):

    * GCC 4.2 or newer (for SpiderMonkey builds 4.7+)
    * Python 2.6 or 2.7
    * GNU Make 3.81 or newer
    * libexecinfo (FreeBSD and OpenBSD only)
    * for SpiderMonkey : 'which' python module (sudo easy_install tools/which-1.1.0-py2.7.egg)

First clone the source codes from Github: (or Download as a .zip file)
```bash
git clone https://github.com/Nubisa/jxcore.git
```

To compile with  

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

After a successful compilation process, you should have jxcore installed into `/jxcoreSM/bin` or `/jxcoreV8/bin` folder (depending on the engine selection). 

JXcore internal JavaScript files can be embedded in two ways (compressed, or as-is). If you are planning to use `jx` binary for a native package creation, we advice you to use the `compressed` build.

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

**Compile as a Static Library** 
You can compile JXcore as a `static library` and embed it into your solution.

Simply add `--static-library` parameter to one of the above `configure` definitions. You should have the compiled lib files inside the target installation folder.

**Notes**

If your python binary is in a non-standard location or has a
non-standard name, run the following instead:

    export PYTHON=/path/to/python
    $PYTHON ./configure
    make
    make install

Windows:

    vcbuild.bat

You can download (latest stable) binaries for various operating systems from  
[http://jxcore.com/downloads/](http://jxcore.com/downloads/)  
(Future `jxcore.io` releases will be shared from `http://jxcore.io`)

### Known issues 

In addition to reported ones, the list of issues we are working on is available from [here](https://github.com/Nubisa/jxcore/blob/master/known_issues.md)

Feel free to help us to implement them sooner! If you have a different opinion, let us know from `issues` section. 

### To run the tests:

See [TESTING.md](doc/TESTING.md)