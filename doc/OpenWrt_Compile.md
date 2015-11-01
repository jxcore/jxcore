### OpenWrt JXcore Package Creation

OpenWrt is a GNU/Linux based firmware program for embedded devices such as residential gateways and routers.

In order to run a Node.JS application on OpenWrt targets, you need to create your own 
package for the target device. We made package creation steps are very easy, **however you 
need a Ubuntu machine or VM for it. OpenWrt SDK is compatible with Ubuntu OS.**

Copy and paste the script below into terminal;
```bash
curl https://raw.githubusercontent.com/jxcore/jxcore/master/build_scripts/openwrt/install.sh | bash
```

This script will be preparing the environment for OpenWrt-JXcore package creation. 
It will also install the latest JXcore-V8 release on your machine.

When the process is completed, you should see a message similar to below;

> Use 'wrt-jx/create_package.js <link to OpenWrt SDK> <arm or mipsel>'

At this point you need to find a link to your device's SDK from [OpenWrt downloads page](https://downloads.openwrt.org)
i.e. for Asus n14u, we picked the SDK from [this](https://downloads.openwrt.org/barrier_breaker/14.07/ramips/mt7620n/) page

Sample Link (make sure your link also starts with OpenWrt-SDK)
> https://downloads.openwrt.org/barrier_breaker/14.07/ramips/mt7620n/OpenWrt-SDK-ramips-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2

Once you find the link, use it as shown below;

> cd wrt-jx
> ./create_package.js [THE LINK for OpenWrt-SDK] [DEVICE-ARCH]

For [DEVICE-ARCH] use either `arm` or `mipsel` based on your device's chipset. i.e. the sample device has `mipsel`

This operation may take a little bit time (depends to your Internet connection speed). 
Eventually you should have the text message below;

> OpenWRT package builder is ready  
Visit OpenWrt-SDK folder and run 'make V=s'

So the last step is compiling your package;
```
cd OpenWrt-SDK
make V=s
```

This last part is going to take a while. It will cross compile JXcore for your OpenWrt target.

Once it's done, you will see a text for the location of JXcore package.

>


Apart from this `.ipk` package you should also download three other packages from OpenWrt's download page.
Remember the page you have found the link for `OpenWrt-SDK` file. Go back to same page and click to 
`packages` and then `base` links.

You should find the packages below and download them on your computer.
```
libpthread...
librt..
libstdcpp..
```

Once you have all the `ipk` files, copy all of them to your device. In order to install a `ipk` package;

```
opkg install <ipk file>
```

You are expected to install all three packages before installing `jxcore` package.

### Optional

By default Sqlite and Leveldown are not embedded into OpenWrt JXcore packages. 
If you want one of these database engines are embedded into your JXcore build, 
visit `OpenWrt-SDK/package/jxcore/Makefile` and update `./configure` part as 
mentioned from [this page](https://github.com/jxcore/jxcore/blob/master/doc/HOW_TO_COMPILE.md#additional-keys)
