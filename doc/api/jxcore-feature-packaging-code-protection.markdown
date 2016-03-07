# Packaging

JXcore introduces a unique feature for packaging of source files and other assets into JX packages.

Let’s assume you have a large project consisting of many files. This feature packs them all into a single file to simplify the distribution.

JX packages can be easily executed with JXcore, just like regular JavaScript applications:

    > jx helloWorld.jx

instead of:

    > jx helloWorld.js

## Command

### package

    > jx package javascript_file [name_of_the_package] [options]

The `jx package` command recursively scans the current folder and generates a `JXP` package information file based on all files in that directory.
After that, it compiles the `JXP` file (by invoking `compile` command).

* `javascript_file` - the main file, which will be executed when JX package is launched with JXcore.
* `name_of_the_package` - indicates the name of the package file. For example, giving the value *MyPackage*  will create *MyPackage.jx* file.
This value is optional. When not provided, the package name will be evaluated from `javascript_file` parameter (file name without an extension).
Also the command-line argument [--name](#name) may be used here explicitly.

Suppose you have a simple project, with just two files: *helloWorld.js* and *index.html*. When you call:

    > jx package helloWorld.js "HelloWorld"

or equivalent:

    > jx package helloWorld.js --name "HelloWorld"

initially, the tool generates `JXP` project file (*HelloWorld.jxp*). Then it is used as an input for `compile` command,
which will create the output JX package *HelloWorld.jx*.

#### options

You may specify none, one or more of the following for the `jx package` command:

* --add [ file||folder [, file2||folder2, ... ]]
* --slim file||folder [, file2||folder2, ... ]
* --native
* --show-progress
* --sign
* JXP fields may be also provided here. See [File structure](#file-structure) for more information.

All of the options may be provided with double dash prefix (e.g. `--add`) which satisfies the general convention for long name parameters.
However for backwards compatibility, the single dash (e.g. `-add`) is still supported.

#### boolean values

Some of the switches may be provided as boolean values. Below is description of their usage.

- if not provided, uses the default value. Each of the boolean switches described below has its own mentioned default value.

- if provided, but not followed by any value, it acts as `true`:

```bash
> jx package helloWorld.js --extract
```

- if provided, and followed by `0` or `no` or `false`, it acts as `false`:

```bash
> jx package helloWorld.js --extract 0
> jx package helloWorld.js --extract no
> jx package helloWorld.js --extract false
```

- if provided, and followed by anything else than `0` or `no` or `false`, it acts as `true`:

```bash
> jx package helloWorld.js --extract 1
> jx package helloWorld.js --extract yes
> jx package helloWorld.js --extract something
```

#### --add

This optional parameter followed by file and/or folder names separated with commas - **explicitly adds** those files/folders into the final JX package.
For example, you may want to package only certain files/folders located at current directory - not the whole its contents.

The default separator is a comma sign. However you may use any other separator by setting a value of special environment variable [JX_ARG_SEP](jxcore-utils.markdown#jxargsep).

If you want to pack just one file (e.g. *helloWorld.js*) you can provide an `--add` option without a file name.
Thus the following two commands are equivalent:

     > jx package helloWorld.js "HelloWorld" --add

     > jx package helloWorld.js "HelloWorld" --add helloWorld.js

Yu can still combine `-add` and `-slim` together, e.g. to add a folder, but exclude its sub-directory, like:

     > jx package helloWorld.js "HelloWorld" --add node_modules --slim node_modules/express

#### --slim

This optional parameter followed by file and/or folder names separated with commas - **prevents adding** those files/folders into the final JX package.

The default separator is a comma sign. However you may use any other character by setting special environment variable [JX_ARG_SEP](jxcore-utils.markdown#jxargsep).

##### wildcards

For both `--add` and `--slim` you can also use wildcards (`*` and `?`) for each file/folder entry.
However if you do so, you'd better wrap them in double quotes, like below:

    > jx package helloWorld.js "HelloWorld" --add "file*.txt"

Otherwise the wildcard expression would be evaluated by shell (before invoking the command) and `--add` option
would receive only first of the matched entries.

Separated entries are also valid:

    > jx package helloWorld.js "HelloWorld" --add "file*.txt,*.jpg" --slim "node?modules,dir*"

##### absolute and relative paths

Each single entries provided to `--add` or `--slim` may represent either an absolute path or path relative to current working directory.
Below example defines for the `--slim` option the same path in 3 ways (2 relative and 3rd absolute), which is of course redundant, however illustrates the subject:

     > jx package helloWorld.js "HelloWorld" --slim out,./out,/users/me/folder/out

#### --native

Boolean value. Default is `false`. See also [boolean values](#boolean-values).

     > jx package helloWorld.js "HelloWorld" --native

When it's set to `true`, the compilation process creates standalone, self-executable binary rather than a package.
It means, that you can run it directly without `jx` binary.

Also, the output file name will be changed. It will no longer contain *.jx* extension.
In fact, for Unix systems it will not contain any extension at all, while on Windows - it will be an *.exe*.

Thus, you can run it on Unix systems the following way:

    > ./helloWorld

On Windows:

    > helloWorld.exe

Additionally on Windows platforms certain file description details are written into the package's header information.
Those are: `--company`, `--copyright`, `--description`, `--name` and `--version`.

#### --show-progress

This parameter defines the way of displaying packaging progress.
It may receive multiple values: `list` (default), `line`, `percent`, `none`.

Each option also has it's own influence on the performance of packaging process, which gets more visible for larger projects (containing e.g. thousands of files).

* `list` - the standard way of displaying added files as a list - one below another.
For really large projects it may not only flood the console window, but also unnecessarily extend the packaging duration.

* `line` - not much faster then `list`, however doesn't flood the screen - each of the next added file is displayed in the same line of console window.

* `percent` - only the percent of the progress is displayed, instead of file names.
This option may significantly reduce packaging duration, because the progress is printed only 100 times (100%), instead of e.g. 10 000 (in case if you would have a project containing that many files).

* `none` - the packaging process doesn't display packaging progress at all. For large projects it is expected to be the fastest.

    > jx package helloWorld.js "HelloWorld" --show-progress percent

#### --sign

String value. It it used only when [-native](#--native) switch is set to `true`.
It can be used for signing the native executable with user's certificate after the package is created.

This applies only for Windows platforms and can work only if [Sign Tool](https://msdn.microsoft.com/en-us/library/8s9b9yaz%28v=vs.110%29.aspx) is installed in the system
(it is s automatically installed with Visual Studio).

The `--sign` switch may receive few variations of values:

* **no value** (which means `true`)

For example:

    > jx package helloWorld.js "HelloWorld" --native --sign

This internally invokes the following command:

    > signtool sign /a HelloWorld.exe

which automatically selects the best signing certificate. Please refer to `signtool sign /?` help.

* **file path of user's certificate**

For example:

    > jx package helloWorld.js "HelloWorld" --native --sign "c:\mycert.pfx"

This internally invokes the following command:

    > signtool sign /f "c:\mycert.pfx" HelloWorld.exe

Signs the native package with provided certificate file.
However this will not work if the certificate requires a password, because it needs to be specified explicitly.
In this case you can use the next approach.

* **signtool's command-line parameters**:

For example:

    > jx package helloWorld.js "HelloWorld" --native --sign "/f 'c:\mycert.pfx' /p password"

This internally invokes the following command:

    > signtool sign /f "c:\mycert.pfx" /p password HelloWorld.exe

### compile

When you already have a `JXP` project file (either created with `package` command or manually), you can call `compile` for generating a JX package.

    > jx compile project_file.jxp -native

When `--native` switch is provided with `jx compile` command, it overrides `native` parameter value from a `JXP` file.

## Hiding body of functions

As of JXcore v Beta-0.3.0.0 (open source version) this feature is no longer available.

## JX package

### About JX package file

The JX package file is what you get as a result of compilation and packaging your project.
It’s a binary file used only by `jx` executable.
Contains all of the script files of your project, as well as assets, which can be considered as static resources.

### Compiling

See `compile` command.

### Launching

JX packages can be executed as follows:

    > jx my_project.jx

Obviously, you need to have JXcore installed first. For this, please visit [jxcore/jxcore-release](https://github.com/jxcore/jxcore-release) page.

You can also run the package in multiple instances.

    > jx mt my_project.jx

or

    > jx mt-keep my_project.jx

## JXP project file

The JXP file is a JX package description. It contains information about the package.
This is also the input file for the compilation of JX file.
It means, if you want to package your project into a JX package, you need to create JXP project file first.

You can do it either manually or by using `package` command.

### Excluding folders

See `package` command with `--slim` switch.

### File structure

The JXP project file is a simple text file that contains package description written as json literal object:

```js
{
    "name": "HelloWorld",
    "version": "1.0",
    "author": "",
    "description": "",
    "company": "",
    "copyright": "",
    "website" : "",
    "package": null,
    "startup": "helloWorld.js",
    "execute": null,
    "extract": {
        "what" :  "*.node,*.txt",
        "where" : "my_folder",
        "message" : "Extracting now...",
        "verbose" : true,
        "overwrite" : true,
        "chmod" : false
    },
    "output": "helloWorld.jx",
    "files": [
        "helloWorld.js"
    ],
    "assets": [
        "index.html"
    ],
    "library": false,
    "license_file": null,
    "readme_file": null,
    "preInstall" : [
        "mkdir new_folder"
    ],
    "fs_reach_sources": true,
    "native" : true,
    "sign" : ""
}
```

You can access this object in a runtime of your JX package by:

```js
var obj = exports.$JXP;
```

And the single field:

```js
var name = obj.name;
```

Below you can find explanation for all supported fields:

#### name

String value.
Can be also used from the command-line: `--name`.

This parameter is mandatory in JXP file, however optional in command-line. When not provided, the package name will be evaluated
from `javascript_file` parameter (file name without an extension) - in this case of example above, this would be "helloWorld".

Hence the two following calls are equivalent:

    > jx package helloWorld.js --name MyApp
    > jx package helloWorld.js MyApp

#### version

String value. Default "1.0".
Can be also used from the command-line: `--version`, e.g.:

    > jx package helloWorld.js --version 2.1

#### author

String value. Optional.
Can be also used from the command-line: `--author`, e.g.:

    > jx package helloWorld.js --author "John Doe"

#### description

String value. Optional.
Can be also used from the command-line: `--description`, e.g.:

    > jx package helloWorld.js --description "My best app"

#### company

String value. Optional.
Can be also used from the command-line: `--company`, e.g.:

    > jx package helloWorld.js --company "My company"

#### copyright

String value. Optional.
Can be also used from the command-line: `--copyright`, e.g.:

    > jx package helloWorld.js --copyright "My company"

#### website

String value. Optional.
Can be also used from the command-line: `--website`, e.g:

    > jx package helloWorld.js --website "http://mydomain.com"

#### startup

Name of the main project file. If execute parameter is not defined, this file will be executed first when you run the package.

#### execute

Name of the main execution file. If this parameter is omitted or null – the value from startup will be used.
This parameter has different meaning depending on the `library` value.
When the package is compiled with `library` = `false`, and you run the compiled package, this `execute` file will be executed first.
If `library` is `true`, and the package is called with `require()` method, the execute file will be returned by the latter.

#### extract

This parameter may receive either boolean value, e.g:

```js
"extract" : true
```

or an object with extended data:

```js
"extract" : {
    "what" : [
        "*.node",
        "*.txt",
        "templates"
    ],
    "where" : "my_folder",
    "message" : "Extracting now...",
    "verbose" : true,
    "overwrite" : true,
    "chmod: false
}
```

Default value for `extract` is `false` (or when parameter is not provided).

When it's set to `true` or an object, all package contents will be extracted at first run of the compiled package.
By default there will be a new folder created with the `name` parameter.
All files and assets embedded inside the package will be saved with full directory structure preserved.

##### chmod

Boolean value. Default is `true`.
Can be also used from the command-line: `--extract-chmod`. See also [boolean values](#boolean-values).

    > jx package helloWorld.js --extract --extract-chmod false

When it's omitted or set to `true`, the package extraction preserves original chmod attributes of packaged files and folders.

When it's `false`, the packaging process will not even collect/store in package mode values for directories thus for larger projects this may result in slightly smaller package sizes.

##### message

You can display a custom message before the extraction starts. For this use the `message` parameter. It can be a string or an array.
When providing an array, you may benefit from `jxcore.utils.console.log()` formatting feature. For example:

```js
"message" : [ "Extracting now...", "red+bold" ]
```

The message can be also set through the command-line parameter: `--extract-message`, e.g:

    > jx package helloWorld.js --extract --extract-message "Extracting now..."

##### overwrite

Boolean value. Default is `false`.
Can be also used from the command-line: `--extract-overwrite`. See also [boolean values](#boolean-values).

    > jx package helloWorld.js --extract --extract-overwrite

When it's set to `true`, the package extraction overwrites any existing files.

##### pre-actions

Analogous to [preInstall](#preinstall) option, except that here you can define system commands to be executed right **before jx package extraction**, rather than execution.
Commands are executed in the same order as the array is defined.

The special keyword `JX_BINARY` is also respected and is replaced during the runtime with current `jx` executable path.

If [verbose](#verbose) is also defined, than each of the pre-action step will be logged into the console window, otherwise only errors will be displayed.

When providing this parameter from the command-line, use `--extract-pre-actions`:

```bash
> jx package helloWorld.js --extract-pre-actions "mkdir -p testfolder, JX_BINARY -jxv" --extract-verbose
Executing `extract-pre-actions` steps:
1. 	 mkdir -p testfolder ... OK
2. 	 JX_BINARY -jxv ... OK
v Beta-0.3.0.6
```

The above set of command options will get converted into the following `extract` object in .JXP file:

```js
"extract": {
    "pre-actions": [
        "mkdir -p testfolder",
        "JX_BINARY -jxv"
    ],
    "verbose": true
}
```

##### post-actions

Same as [pre-actions](#pre-actions), except that here you can define system commands to be executed right **after** jx package extraction.
Can be also used from the command-line: `--extract-post-actions`.

##### verbose

Boolean value. Default is `false`.
Can be also used from the command-line: `--extract-verbose`. See also [boolean values](#boolean-values).

    > jx package helloWorld.js --extract --extract-verbose

Value set to `true` displays the list of files being extracted.

##### Full Extraction

When the `extract` in JXP file parameter is boolean `true`, it enables the **full extraction** (extracts the entire contents on first package execution).

You can achieve the same by using command-line parameter `--extract`, which accepts multiple [boolean values](#boolean-values), e.g.:

    > jx package helloWorld.js --extract

##### Partial extraction

##### what

The `what` parameter enables **partial contents extraction**. It is an array defining which paths or masks should be extracted.

```js
"extract" : {
    "what" : [
        "*.node",
        "*.txt"
    ]
}
```

The partial extraction may work only if the contents is extracted into the application's root directory,
thus the `where` parameter needs to be set with "./" value.

When providing this parameter from the command-line, use `--extract-what`:

    > jx package helloWorld.js --extract-what "*.node,*.txt"

The default separator is a comma sign. However you may use any other character by setting special environment variable [JX_ARG_SEP](jxcore-utils.markdown#jxargsep).

##### where

The `where` allows to change a folder name into which the package is extracted.
By default, when the value is not provided, it is set to the name of the package.
You may change it into any other name or path (relative to application root directory), for example:

```js
"extract" : {
    "where" : "my_folder/my_sub_folder"
}
```

If you want to extract the contents into the application's root directory rather than into default sub folder, use `"where" : "./"`.

When providing this parameter from the command-line, use `--extract-where`:

    > jx package helloWorld.js --extract-where my_folder

There is also an extra command-line parameter available: `--extract-app-root` which is an alias to `--extract-where "./"`'
Thus the two following calls are equivalent:

    > jx package helloWorld.js --extract-where ./
    > jx package helloWorld.js --extract-where --extract-app-root

#### output

Name of the output JX package.

#### files

This is an array, where you can define, which script files from your project will be included into the JX package.
Only `*.js` and `*.json` files are allowed here.

#### assets

This is the array with static resource files. You can embed any asset file into the `jx` package.

#### library

Boolean value. Default is `true`.
Can be also used from the command-line: `--library`. See also [boolean values](#boolean-values).

Value set to `true` means that JX package can be treated as a library and it can be used from inside another JX package (with `require()` method).
Setting this value to `false` is a good way of preventing its usage as an external module (and then `require()` will not be possible).

#### licence_file

Name of the file containing the licensing information – it is generally a simple text file.
If this parameter is omitted or null and if a file named “LICENSE” exists in the directory from where you compile the package – it will be embedded automatically.

#### readme_file

Name of the file containing additional notes about the package.
If this parameter is omitted or null and if a file named “README” or “README.md” exists in the directory
from where you compile the package– it will be embedded automatically.
When a license or readme file is specified, it can be also displayed in a console window directly from the package.

For example, running the following command:

    > jx package.jx license

will display the licence file to the console without executing the package. The same applies to:

    > jx package.jx readme

#### preInstall

This is an array, where you can define system commands to be executed right before jx package execution.
For example, this might be creating a folder, installing an additional package/module or just anything.
Commands are executed in the same order as the array is defined.

There is a special keyword `JX_BINARY` which is replaced during runtime with current `jx` executable path.

For example, we have the following commands in JXP file:

```js
"preInstall" : [
    "which JX_BINARY > log.txt",
    "JX_BINARY -jxv >> log.txt"
]
```

Those commands will be executed for the first time, when we run the package:

    > jx package.jx

or

```js
var module = require("./package.jx");
```

In this example, the first commands writes the full path string of jx binary to the *log.txt* file, while the second one - executes `jx -jxv`
and appends the result (the jx version number) to the same file.

When all of the commands are executed, there will be a file created `your_module.installed` preventing subsequent execution of pre-install section.
If you want to run it again, simply remove that file.

This parameter can be also used from the command-line: `--preInstall` or `--preinstall`.
In such case it receives commands separated with commas.
However you may use any other character by setting special environment variable [JX_ARG_SEP](jxcore-utils.markdown#jxargsep).

For example, the following command line:

    > jx package helloWorld.js --preinstall "mkdir dir1,touch dir1/file.txt"

will get converted to the following array and embedded into JXP project file:

```js
    ...
	"preInstall": [
		"mkdir dir1",
		"touch dir1/file.txt"
	],
	...
```

#### custom_fields

You can also define your own constants, as many as you want, for example:

```js
{
    // ...
    // ...
    // ...
    field1 : "one",
    myField2 : "two",
    someObject : {
        PI : 3.14159265359,
        userArray : [ 1, 2 ]
    },
    Release : true
}
```

Similarly, accessing those values in a runtime of your JX package is also easy:

```js
var pi = exports.$JXP.someObject.PI;
```

One of the usage examples for those custom fields can be:

```js
if (exports.$JXP.Release) {
    console.log("This is the final release of the product.");
} else {
    console.log("This code is still under development.");
}
```

However, `files` members are not accessible from exports.$JXP.

#### fs_reach_sources

Boolean value. Default is `true`.

Normally, `fs` can not reach the JavaScript files inside the package.
If you need to access all the JavaScript files using `fs` module, you should set this parameter to 'true'.
Otherwise you can either set it to 'false' or give the list of files expected to be reachable by 'fs' module
(i.e. { "lib/testfile.js":true, "lib/test2.js":true } ).

This parameter can be also used from the command-line: `--fs_reach_sources` as one of [boolean values](#boolean-values).

#### native

See the [--native](#--native) command-line switch.

#### sign

Ability to sign the native executable package. See the [`--sign`](#--sign) switch.

### Supported file types

You can embed any asset file (text and binary) into the `jx` package and it will be
placed automatically into the `assets` array of `JXP` project file during execution of `jx package` command.

However, there are two file types, which are treated by JXcore as source files rather than assets:

* js
* json

The difference is, that source files cannot be read from a package during runtime.
This is a security feature of JXcore packaging.
For more information see [Accessing Files and Assets from a Package](#files).

The `jx package` puts them into the `files` array of `JXP` project file during execution of `jx package` command.

Even if you would edit the `JXP` project file manually and add `js` files into the `assets` array, JXcore removes them from the `assets` and treats them as source files.

## Accessing Files and Assets from a Package

### assets

There are a few native methods in the [FileSystem](fs.markdown) module that you can use to access assets embedded inside a JX package.

* `readFile()`
* `readFileSync()`
* `readdir()`
* `readdirSync()`

When called, each of these methods tries to find a given file or a folder in the real file system in the first place.
If nothing is found, JXcore searches through assets from the JX package.

Assets can always be referenced relatively to `__dirname` or `./`.

Let's consider a JX package containing the following directory structure:

- folder
  - file2.html
  - module.js
- index.html
- package.json
- test.js

After creating a package with the command:

    > jx package test.js my_package

we can access the assets of the package from inside the *test.js* file, and this would be like this:

```js
var fs = require('fs');

// gets index.html file from package's assets, if it does not exists on the disk.
var index = fs.readFileSync(__dirname + '/index.html');

// asset from subfolder:
var file = fs.readFileSync(__dirname + '/folder/file2.html');
```

The following sample works as expected, returns the directory contents of the file system. `readdir()` and `readdirSync()`
return from the JX package only when the given path does not exist on real file system. Calling one of these methods for the main
folder of the JX package will return the results from actual file system. In order to reach asset files using one of these
methods, you should consider putting them in a sub folder.

```js
var fs = require('fs');
var index1 = fs.readdirSync('./folder');
```

### files

`readFile()` and `readFileSync()` methods can be used to reach any files inside a JX package except for the source files.
JavaScript files inside a JX package can be accessed only by using `require()` method. In other words,
you can not read the source files using `readFile()` or `readFileSync()`.

Example below shows how to load *module.js* file contained inside *folder* directory of JX package:

```js
var lib = require("./folder/module.js");
console.log("lib.value", lib.value);
```


