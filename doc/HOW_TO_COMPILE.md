Prerequisites (Unix only):

    * GCC 4.2 or newer (for SpiderMonkey builds 4.7+)
    * Python 2.7.x
    * GNU Make 3.81 or newer
    * libexecinfo (FreeBSD and OpenBSD only)
    * for Windows (VS2012+) and Visual C++ Redistributable

First get the source codes from Github: (or Download as a .zip file)
```bash
git clone https://github.com/jxcore/jxcore.git
```

If all you want to compile JXcore for mobile platforms, you may want to jump into 
[Android](Android_Compile.md), or [iOS](iOS_Compile.md) compilation details.

To compile for desktop/server environments:

##### SpiderMonkey
```bash
./configure --engine-mozilla
make
```

##### V8
```bash
./configure
make
```

> JXcore 0.3.x uses V8 3.14.x by default. You may set V8 engine version by `--engine-v8-3-28`
 If your application depends on native addons (C,C++), you should be using V8 3.14.x
 
##### V8 3.28
```
./configure --engine-v8-3-28
make
```

Windows:
```
/ $> vcbuild.bat
```

Windows ARM:
```
/ $> vcbuild.bat arm --engine-chakra
```

To compile with SpiderMonkey, `--engine-mozilla` key also applies to Windows builds. 
i.e. `vcbuild.bat --engine-mozilla`

JXcore also supports Chakra engine on Windows10+. Use `--engine-chakra` as shown above. With chakra engine JXcore has built-in uwp support (through `jxcore.uwp` property).

After a successful compilation process, you should have jxcore installed into `/jxcoreSM/bin` 
or `/jxcoreV8/bin` folder (depending on the engine selection). (Check Release folder on 
Windows)

JXcore internal JavaScript files can be embedded in two ways (compressed, or as-is). If you 
are planning to use `jx` binary for a native package creation, we advice you to use the 
`compressed` build. 

!! In order to use the `compressed` build, you should have a `jx` binary ready on your 
platform. You may compile the non-compressed version first and then `compressed` one second.

##### SpiderMonkey and Compressed Internals
```
./configure --prefix=/jxcoreSM --engine-mozilla --compress-internals
make install
```

##### V8 and Compressed Internals
```bash
./configure --prefix=/jxcoreV8 --compress-internals
make install
```

Windows;
```
/ $> vcbuild.bat --compress-internals
```

Windows (Chakra engine);
```
/ $> vcbuild.bat --engine-chakra --compress-internals
```

##### Compile as a Static Library 
You can compile JXcore as a `static library` and embed it into your solution.

Simply add `--static-library` parameter to one of the above `configure` definitions. You 
should have the compiled lib files inside the target installation folder. 

On Windows;
```
/ $> vcbuild.bat --static-library
```

##### Compile as a Dynamic Library

You can also compile JXcore as a `dynamic library`, also known as a `shared library` or 
`DLL`, and distribute it alongside your solution.

Simply add `--shared-library` parameter to one of the above `configure` definitions. 

On Windows;
```
/ $> vcbuild.bat --shared-library
```

On OS X, an extra step is needed after building, because the .dylib file contains information 
on where it should be found when needed:

```
install_name_tool -id /path/to/built/libjx.dylib
```

Also, if you would like to embed the library in your OS X application, you should add an 
extra build step as a `Run script phase` to your app in Xcode, to modify the executable so 
that on run the .dylib would be searched for in the app bundle, not in a global location. 
For example:

```
install_name_tool -change /usr/local/lib/libjx.dylib @executable_path/../Library/libjx.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME"
```

##### Additional keys

--no-sqlite : do not embed sqlite3 (by default JXcore embeds sqlite)

--embed-leveldown : embed leveldown engine into JXcore (by default JXcore doesn't embed 
leveldown)

> You need to init git submodule to compile JXcore with leveldown embedded
```
git submodule init
git submodule update
```

> For leveldown-mobile API, see [https://github.com/Level/leveldown-mobile](https://github.com/Level/leveldown-mobile)


#### Notes
If your python binary is in a non-standard location or has a non-standard name, 
run the following instead:

    export PYTHON=/path/to/python
    $PYTHON ./configure
    make
    make install

#### Download JXcore 
You can download (latest stable) binaries for various operating systems from  
[jxcore/jxcore-release](https://github.com/jxcore/jxcore-release)
(Future `jxcore.io` releases will be shared from `http://jxcore.io`)

#### Notes for Windows

If you run `vcbuild.bat` without specifying the target architecture, it will be determined 
by a current Python version you have installed (not the operating system architecture).
Thus, if you have Python x86 installed on Windows x64, then `vcbuild.bat` will build x86 
JXcore binaries (instead of probably expected JXcore x64).
To build JXcore x64 on Windows x64, you need to make sure, that you use Python installer for 
x64 platforms, e.g. `python-2.7.9.amd64.msi`.

Then you can still build JXcore x32 on Windows x64:

```
/ $> vcbuild.bat ia32
```

**For ARM build use** `--engine-chakra` option. 

```
/ $> vcbuild.bat arm --engine-chakra
```
When you build for `ARM` build script also includes `leveldown` native database support.

#### Notes for CentOS/Red Hat

If you'll ever have problem with GCC 4.7+ installation on Red Hat, please see 
[#297](https://github.com/jxcore/jxcore/issues/297).

#### Compiling against Mipsel

Your linux distro might be using an older version of GCC. We encourage you to set 
`--dest-os=mipsel` explicitly within the  `./configure` call

```
./configure ...... --dest-cpu=mipsel
```