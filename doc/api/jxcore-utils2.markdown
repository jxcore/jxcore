# Utils

Api methods described in this section are accessible any time within JXcore application from a global object:

```js
var utils = jxcore.utils;
```

## Flow Control

JXcore exposes few methods to provide enhanced control over the application - pausing and resuming execution of the code.
This feature may be compared to `sleep()` function from other languages.
There is also a blog post on this subject: [Pause, Jump, and Continue](http://jxcore.com/pause-jump-and-continue/).

### pause()

Stops the execution of JavaScript exactly on the line called.
It also stops processing all new IO events which may occur from now on. To be more precise, it doesn't poll from libuv events.
All events are received, but kept in libuv queue and stay unprocessed.
To resume the execution of the application and start processing the events, you need to call `continue()`.

Below is an example of a simple `sleep` method implementation:

```js
var sleep = function(timeout){
    setTimeout(function(){ jxcore.utils.continue(); }, timeout);
    jxcore.utils.pause();
};
```

Then you can just call it:

```js
console.log("Before Sleeping");
sleep(2000);
console.log("Hello Again!");
```

Please remind that actual pause may be somewhat longer (difference of few milliseconds) since pausing time depends on how busy is the system / current process.
For `sleep(2000)` this probably would not make a difference. But if you call `sleep(1)` - it may wait a little bit longer.

### jump()

This function acts similar to `pause()` except that it doesn't stop processing IO events.

### continue()

Resumes block execution paused by `pause()` or `jump()`;


## Console Output

### console.log(data [, ...], color)

This method of `jxcore.utils` object does the same thing as native [`console.log()`](console.markdown#console-log-data)
(prints the output to stdout with newline) except that it accepts a color name as the last parameter.

The `color` parameter is not limited only for colorizing the output - it may also apply some simple formatting.
Value of this parameter may be one of the following:

* RGB Colors
    * red
    * green
    * blue
    * white

* CMYK Colors
    * cyan
    * magenta
    * yellow
    * black

* Text Formatting
    * bold
    * underline

```js
var clog = jxcore.utils.console.log;

clog("this is green", "green");
clog("this", "is", "also", "green", "green");
clog("but", "this", "one", "is bolded", "bold");
```

If environment variable `NODE_DISABLE_COLORS` is set to 1, which disables colors in the REPL (see `jx -h`),
the last parameter `color` is printed to the output just like `data` argument. For example:

```js
jxcore.utils.console.log("this is green", "green");
```
displays:
> this is green green


### console.write(data [, ...], color)

Same as `console.log()` except that it does not add a newline character at the end of the data.

This way you can combine different colors and formats within the same line:

```js
var cw = jxcore.utils.console.write;
cw("bolded", "bold");
cw("normal");
cw("bolded", "again", "bold");
```

Displays:

> **bolded**normal**bolded again**


### setColor(data, color)

Returns the `data` string with specific `color` attribute added.
Sending this string to the console will result in colorized output.

```js
var yellow = jxcore.utils.console.setColor("The yellow is here", "yellow");
console.log(yellow);
```

## System Info

### getCPU(cb, timeout)

* `cb` {Function} - callback listener
    * `percent` {Number}
    * `timeout` {Number}
* `timeout` {Number} - probing time in milliseconds

Adds `cb` callback listener that will be invoking regularly with a `timeout` interval.
As an argument it will receive `percent` amount of current CPU usage.

Below example shows how to read CPU usage every 1 second:

```js
jxcore.utils.getCPU(function (percent, timer) {
    jxcore.utils.console.log("%d%% CPU Percent Used in last %d ms",
        percent, timer , "green");
}, 1000);
```

Beware that this method only supports a single listener instance at a time.
In order to clear the previous listener, call the method without any parameters:

```js
jxcore.utils.getCPU();
```

### OSInfo()

This method is complementary to [`process.platform`](process.markdown#process-platform) and [`process.arch`](process.markdown#process-arch).
It returns set of boolean values for the current system.

```js
console.log(jxcore.utils.OSInfo());
```

For a MAC x64 it would return:

```js
{ fullName: 'mac-x64',
  isUbuntu: false,
  isDebian: false,
  isMac: true,
  is64: true,
  is32: false,
  isARM: false,
  isRH: false,
  isSuse: false,
  isBSD: false,
  isArch: false,
  isWindows: false,
  OS_STR: 'osx64' }
```

## Others

### smartRequire(moduleName)

Same as `require()`, except that it installs packages (the ones which are not installed) from NPM during the runtime.
If the module is not found on the file system, `smartRequire()` installs it first and then invokes regular `require()` function.

For example, let's consider using an npm's `express` package.

Normally you would install it first:

```bash
> jx install express
```

and then application would call:

```js
var express = require('express');
```

With `smartRequire()` it is much more simpler:

```js
var smart_require = jxcore.utils.smartRequire(require);

// express package doesn't have to be installed
var express = smart_require('express');
// and it was installed just now
```

### uniqueId()

Returns an integer number unique for current application instance.
Each call of this method increments the counter and returns value greater by 1 (starting from 0).

```js
for(var o=0; o<3; o++) {
    console.log(jxcore.utils.uniqueId());
}
```

The above example displays:

```js
0
1
2
```

Restarting the application will make the numbers count from 0 again.