
The pre-compiled binaries of JXcore available from [jxcore.com/downloads](http://jxcore.com/downloads).
You can also use bash script (for Unix platforms), or setup (for Windows) that can do the installation 
for you.

# For Linux/OSX:

The [jx_install.sh](https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh) script always 
downloads the latest release.

```bash
$ curl https://raw.githubusercontent.com/jxcore/jxcore/master/tools/jx_install.sh | bash
```

## Script options

Several options are available for customizing the installation process. 

Example usages given below:

```bash
# with curl:
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash -s force sm local
# by calling the script directly:
$ ./jx_install.sh force sm local
```

### sm

Installs SpiderMonkey build instead of default V8.

```bash
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash -s sm
```

### v8

Specifying the engine for V8 is not necessary (as this is the default engine), but still acceptable. 
Thus both of the following calls are equivalent:

```bash
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash -s v8
```

### local

Installs jx binary into the current directory `./` rather than into global path (by default 
JXcore is installed into global `/usr/local/bin/jx`).

```bash
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash -s local
```

### force

If there is already the same version at target path installed, forces to overwrite the jx file.  
This may be useful in case when you want to switch from one engine to another:

```bash
# installs v8:
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash
# forces to install SpiderMonkey
$ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | bash -s force sm
```

## Notes:

* FreeBSD requires bash, and unzip installed
* If you have `permission denied` message, make sure the user has root access. 
Try executing the command as `su`, or `sudo`:

    ```bash
    $ curl https://github.com/jxcore/jxcore/blob/master/tools/jx_install.sh | sudo bash
    ```

# For Windows:

Apart from separate zip files per each engine/architecture (32sm/64sm/32v8/64v8), there is 
also an installer available.

For example on **Windows x32** you may choose one of the two options:

* V8 x32
* SpiderMonkey x32

While on **Windows x64** you have total 4 options:

* V8 x32
* V8 x64
* SpiderMonkey x32
* SpiderMonkey x64
