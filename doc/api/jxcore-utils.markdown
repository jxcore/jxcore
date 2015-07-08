# Utils

Api methods described in this section are accessible any time within JXcore application from a global object:

```js
var utils = jxcore.utils;
```

## Environment Variables

### JX_ARG_SEP

This variable is used to define a custom character/string used as separator for parsing strings which holds multiple separated values.

For example `-add`, `-slim` and `-preInstall` options in [`jx package`](jxcore-feature-packaging-code-protection.html#package) command or `--autoremove` in [`jx install`](jxcore-command-install.html#install) are using this feature.

The default value (if variable is not set) is a comma sign (`,`). This is how you can change the value:

Unix platforms:

```bash
export JX_ARG_SEP=";"
```

Windows:

```bash
set JX_ARG_SEP=";"
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

The `jxcore.utils.console.log()` method does the same thing as native [`console.log()`](console.markdown#consolelogdata-)
(prints the output to stdout with newline) except that it accepts a color name as the last parameter.

The `color` parameter is not limited only for colorizing the output - it may also apply some simple formatting.
Value of this parameter may be one of the following:

* Foreground Colors
    * red
    * green
    * blue
    * white
    * cyan
    * magenta
    * yellow
    * black
    * grey
    * default

* Background Colors
    * bgRed
    * bgGreen
    * bgBlue
    * bgWhite
    * bgYellow
    * bgMagenta
    * bgCyan
    * bgBlack
    * bgDefault

* Text Formatting
    * bold
    * italic (_not widely supported - may not work on all platforms_)
    * underline
    * inverse

* Other
    * clear

```js
var clog = jxcore.utils.console.log;

clog("this is green", "green");
clog("this", "is", "also", "green", "green");
clog("but", "this", "one", "is bolded", "bold");
```

You may combine those attributes with a plus (`+`) sign, for example:

```js
var clog = jxcore.utils.console.log;
// green foreground with yellow background and all bold:
clog("test", "green+bgYellow+bold");
// bolded underline:
clog("test", "underline+bold");
```

Please note, that you can also use the `clear` attribute when printing the output.
When combining with other attributes, the `clear` attribute should go first.
The following example clears the screen and prints formatted text (red in this case).

```js
jxcore.utils.console.log("my red color", "clear+red");
```

If environment variable `NODE_DISABLE_COLORS` is set to 1, which disables colors in the REPL (see `jx -h`),
the last parameter `color` is printed to the output just like `data` argument. For example:

```bash
# unix systems
$ export NODE_DISABLE_COLORS=1;
```

```js
jxcore.utils.console.log("this is green", "green");
```
displays:
> this is green green

### console.info(data [, ...], [color])

Same as native [`console.info()`](console.markdown#consoleinfodata-), but with color support. If color is not given as last argument, the `green` is used by default.

### console.error(data [, ...], [color])

Same as native [`console.error()`](console.markdown#consoleerrordata-), but with color support. If color is not given as last argument, the `red` is used by default.

### console.warn(data [, ...], [color])

Same as native [`console.warn()`](console.markdown#consolewarndata-), but with color support. If color is not given as last argument, the `magenta` is used by default.

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


### console.clear([data [, ...], color])

This is a special method which clears the current console's output and places the cursor at the top-left corner.
Can be called with or without any arguments.
If the arguments are provided, they are written into stdout with `console.log()` right after the screen is cleared.

For example the following calls are equivalent:

```js
jxcore.utils.console.clear();
jxcore.utils.console.log("my red color", "red");
```

and

```js
jxcore.utils.console.clear("my red color", "red");
```

and

```js
// clear attribute should go first
jxcore.utils.console.log("my red color", "clear+red");
```

### setColor(data [, ...], color)

Returns the string containing all `data` arguments wrapped with specific `color` attribute.
Sending this string to the console will result in colorized output.

```js
var yellow = jxcore.utils.console.setColor("The yellow is here", "and here", "yellow");
console.log(yellow);
```

### removeColors(txt)

Removes from the `txt` string all formatting control codes and returns a new plain string.

```js
// [31mformatted with red[39m
var formatted = jxcore.utils.console.setColor("formatted with red", "red");

// formatted with red
var plain = jxcore.utils.console.removeColors(formatted);
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

This method is complementary to [`process.platform`](process.markdown#processplatform) and [`process.arch`](process.markdown#processarch).
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

## argv Parsing

### parse([argv], [options])

* `argv` {Array} [optional]
* `options` {Object} [optional]
    * `force` {Boolean} - forces reparsing the `process.argv`

If `argv` is omitted, then `process.argv` will be parsed. Otherwise the method expects `argv` to be an array.

The `jxcore.utils.argv.parse()` method (without `argv` provided) parses `process.argv` and is doing that only once - at first call.
Any subsequent `parse()` invocation will just return previously parsed result.
If you need to parse `process.argv` again, you may pass `force` option, or provide `process.argv` as na argument:

```js
// forcing
var parsed = jxcore.utils.argv.parse({ force : true });

// passing as an arg
var parsed = jxcore.utils.argv.parse(process.argv);
```

The function returns an object with parsed arguments. For example:

**test.js**

```js
var parsed = jxcore.utils.argv.parse();
console.log(JSON.stringify(parsed));
```

```bash
$ jx test.js --one=1 --two 2 --three -abc -d 1 str1 str2 str3
{
    "_": {
        "mainModule": "test.js",
        "withoutPrefix": [
            "str1",
            "str2",
            "str3"
        ]
    },
    "one": {
        "index": 2,
        "type": "string",
        "asBool": true,
        "prefix": "--",
        "name": "one",
        "valueSep": "=",
        "value": "1",
        "isInt": true,
        "asInt": 1,
        "hasValue": true
    },
    "two": {
        "index": 3,
        "type": "string",
        "asBool": true,
        "prefix": "--",
        "name": "two",
        "value": "2",
        "isInt": true,
        "asInt": 2,
        "hasValue": true,
        "valueSep": " "
    },
    "three": {
        "index": 5,
        "asBool": true,
        "prefix": "--",
        "name": "three"
    },
    "abc": {
        "index": 6,
        "asBool": true,
        "prefix": "-",
        "name": "abc"
    },
    "d": {
        "index": 7,
        "type": "string",
        "asBool": true,
        "prefix": "-",
        "name": "d",
        "value": "1",
        "isInt": true,
        "asInt": 1,
        "hasValue": true,
        "valueSep": " "
    },
    "str1": {
        "index": 9,
        "type": "string",
        "asBool": true,
        "name": "str1",
        "value": "str1",
        "hasValue": false
    },
    "str2": {
        "index": 10,
        "type": "string",
        "asBool": true,
        "name": "str2",
        "value": "str2",
        "hasValue": false
    },
    "str3": {
        "index": 11,
        "type": "string",
        "asBool": true,
        "name": "str3",
        "value": "str3",
        "hasValue": false
    }
}
```

Each of the parsed arguments has the following members:

* `index` {Number} - position of an argument in `argv`
* `name` {String} - name of an arg stripped of value and leading prefixes, e.g. `--arg1=str1`  (name = "arg1")
* `hasValue` {Boolean} - indicates if an argument has a value provided.
For example `--arg1` has no value, while the following has: `--arg str1`.
* `value` - value of an argument. It may be present in `argv` in multiple ways:
    * `--arg1=str1` or `-b=1`
    * `--add:no` or `-c:3`
    * `--library false  or `-l 0`
* `valueSep` {String} - a separator used in an argument to provide a value: `=`, `:` or space, e.g.: `--arg1=str1`, `-c:3`, `-l 0`
* `prefix` {String} - this may be one of the following: `--`, `-` or `/` on Windows platforms
* `type` {String} - this is evaluated `typeof value`
* `isInt` {Boolean} - `true` if value can be parsed into a Number, e.g. `--count=3`
* `asInt` {Number} - if `isInt` is `true`, than `asInt` contains a parsed Number.
Beware, that it may be `0`, so it should not be always checked with negation: `if (!arg.asInt) {}`.
* `asBool` {Boolean} - it will be `false` only in few cases, when val equals to `no`, `false` or `0` (e.g. `--library false`).
Otherwise it is always `true`, even if there is no value provided, e.g. `-add`.
* `splitBySep(sep)` {Function} - if argument's value contains a separator defined by [jxcore.utils.argv.sep](#sep)
or [JX_ARG_SEP](jxcore-utils.html#jxargsep) this function returns an array with separated parts (split).
It also ignores empty or whitespaced strings. If the result is an empty array or the argument's value does not contain a separator - the function returns `null`.
    * `sep` {String} [optional] - if provided, then will be used instead of [jxcore.utils.argv.sep](#sep)

There is also one special value `_` (`parsed["_"]` in example above), which holds the following fields:

* `withoutPrefix` {Array} - it contains an array of all values passed without any prefix.
This field is introduced mostly for user's convenience/performance (no need to search by iterating through arguments) and is used for example by `jx install` command:

```bash
$ jx install jxm express mongoose
```

In this case `parsed["_"].withoutPrefix` would contain:

```js
    "withoutPrefix": [
        "jxm",
        "express",
        "mongoose"
    ]
```

### sep

Holds a separator value used for argv parsing. Returns a comma sign or an environment variable [JX_ARG_SEP](jxcore-utils.html#jxargsep) if it is set.

## Others

### cmdSync(command)

Executes shell command in synchronous way. It returns an object containing two values: `exitCode` and `out` (the latter is an application's output).

```js
var cmd = "jx -jxv";
var ret = jxcore.utils.cmdSync(cmd);
console.log(ret);
// { out: 'v Beta-0.3.0.0\n', exitCode: 0 }
```

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