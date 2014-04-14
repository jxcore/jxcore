# Packaging & Code Protection

JXcore introduces a unique feature for packaging and encryption of source files and other assets into `JX` packages.

Let’s assume you have a large project consisting of many files. This feature packs them all into a single file to simplify the distribution. It also protects your server side javascript code by encrypting all source files.

`JX` packages can be easily executed with JXcore, just like normal JavaScript applications:

    > jx helloworld.jx

instead of:

    > jx helloworld.js

## Command

### package

    > jx package javascript_file name_of_the_package [-slim folder, folder2, ...]

This command recursively scans the current folder and generates a `JXP` package information file based on all the files in that directory.
After that compiles the `JXP` file (by invoking `compile` command).

* `javascript_file` - the main file, which will be executed when `JX` package will be launched with JXcore.
* `name_of_the_package` - indicates the name of the package file. For example, giving the value *MyPackage*  will create *mypackage.jx* file.
* `-slim` - this optional parameters followed by folder names separated with comma - prevents adding those folders into the final `JX` package.

Suppose you have a simple *Hello_World* project, with just two files: *helloworld.js* and *index.html*. When you call:

    > jx package helloworld.js "Hello World"

initially, the tool generates `JXP` project file (*helloworld.jxp*). Then it is used as an input for `compile` command, which will create the output `JX` package *helloworld.jx*.

### compile

When you already have a `JXP` project file (either created with `package` command or manually), you can call `compile` for generating `JX` package.

    > jx compile project_file.jxp

## Hiding body of functions

As you may already know, a javascript function (like everything in javascript – [w3schools](http://www.w3schools.com)) is an object.
To be more precise, it is an instance of the `Function` which inherits from the `Object`.
And just because the `Object` has defined `toString()` method, you could also call it on some function,
and the result will be a string containing function’s body.

In JXcore Packaging & Code Protection module we solved that problem.
You can hide body of the function inside of a jx package, even if the function is exported out of the module.

### utils.hideMethod(fn)

* `fn` {Function}

Hides body of a given `fn` function. It will remain invokable inside, but

Let’s discuss it by the example. We’ll create *hide_test.js* file with the following code:

```js
exports.myMethod = function (id1) {
    return "this is some function. It returns " + id1;
};
```

And compile it into JX package:

    > jx package hide_test.js "HideTest"

Now if we run it in the command window:

    > jx -e 'console.log(require("./hide_test.jx").myMethod.toString());'

We’ll see the output:

![](http://jxcore.com/wp-content/uploads/2014/02/jx_hiding_body1.gif)

As we can see, the whole content of `myMethod` is currently visible.
But we don’t want it, especially when we talk about code protection.
For this we implemented in JXcore mechanism to hide functions contents, but in the same time to have them still invokable.

Just add the following line to the *hide_test.js*:

```js
jxcore.utils.hideMethod(exports.myMethod);
```

and when you run the package again (after recompiling), the output will be different:

![](http://jxcore.com/wp-content/uploads/2014/02/jx_hiding_body2.gif)

We can still call the method itself:

    > jx -e 'console.log(require("./hide_test.jx").myMethod("an arg"));'

and it displays regular function’s result:

![](http://jxcore.com/wp-content/uploads/2014/01/jx_hiding_body3.gif)

### special comment

As an alternative to `jxcore.utils.hideMethod()` you can also use a special comment `/*_jx_protected_*/` inside a function's body:

```js
var func = function() {
    /*_jx_protected_*/
    console.log("hello");
}
```

It can be placed anywhere, as far it is located between function's curly brackets.

## JX package

### About JX package file

The `JX` package file is what you get as a result of compilation, packaging and encrypting of your project.
It’s a binary file used only by `jx` executable.
Contains all of the script files of your project, as well as assets, which can be considered as static resources.

### Compiling

See `compile` command.

### Launching

JX packages can be executed as follows:

    > jx my_project.jx

Obviously, you need to have JXcore installed first. For this, please visit [Downloads](http://jxcore.com/downloads/) page.

You can also run the package in multiple threads.

    > jx mt my_project.jx

or

    > jx mt-keep my_project.jx

## JXP project file

The JXP file is a JX package description. It contains information about the package.
This is also the input file for the compilation of JX file.
It means, if you want to package your project into a JX package, you need to create JXP project file first.

You can do it either manually or by using `package` command.

### Excluding folders

See `package` command with `-slim` switch.

### File structure

The JXP project file is simple text file that contains package description written as json literal object:

```js
{
    "name": "Hello World",
    "version": "1.0",
    "author": "",
    "description": "",
    "company": "",
    "website" : "",
    "package": null,
    "startup": "helloworld.js",
    "execute": null,
    "extract": false,
    "output": "helloworld.jx",
    "files": [
        "helloworld.js"
    ],
    "assets": [
        "index.html"
    ],
    "library": false,
    "license_file": null,
    "readme_file": null
}
```

You can access this object in a runtime of your `JX` package by:

```js
var obj = exports.$JXP;
```

And the single field:

```js
var name = obj.name;
```

Below you can find explanation for all supported fields:

* **name**, **version**, **author**, **description**, **company**, **website**
These are all string values.
* **package**
This is not supported for now. It is reserved for future use. Using this field, you will be able to specify a path for package.json file, from which the compilation process of JX package will gather all required NPM dependencies and embed them into output JX package.
* **startup**
Name of the main project file. If execute parameter is not defined, this file will be executed first when you run the package.
* **execute**
Name of the main execution file. If this parameter is omitted or null – the value from startup will be used. This parameter has different meaning depending on the library value. When the package is compiled with library = false, and you run the compiled package, this execute file will be executed first. If library is true, and the package is called with require() method, the execute file will be returned by the latter.
* **extract**
It is a boolean value: true or false. When you set this value to true, all package contents will be extracted at first run of the compiled package. There will be a new folder created with the name parameter. All files and assets embedded inside the package will be saved with full directory structure preserved.
* **output**
Name of the output JX package.
* **files**
It is an array, where you can define, which script files from your project will be included into the JX package. Only files with Javascript code should be included there.
* **assets**
This is the array with static resource files. You can embed as an asset any file of the type: html, css, txt, xml, log, types, ini, json.
* **library**
It is a boolean value: true or false. Value set to true means that JX package can be treated as a library and it can be used from inside another JX package (with require() method). Setting this value to false is a good way of preventing its usage as an external module (and then require() will not be possible).
* **licence_file**
Name of the file containing the licensing information – it is generally a simple text file. If this parameter is omitted or null and if a file named “LICENSE” exists in the directory from where you compile the package – it will be embedded automatically.
* **readme_file**
Name of the file containing additional notes about the package. If this parameter is omitted or null and if a file named “README” or “README.md” exists in the directory from where you compile the package – it will be embedded automatically.
When a license or readme file is specified, it can be also displayed in a console window directly from the package.

For example, running the following command:

    > jx package.jx license

will display the licence file to the console without executing the package. The same applies to:

    > jx package.jx readme

* **custom_fields**

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