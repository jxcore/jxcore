# Module Installer

JXcore provides you with an easy and convenient way for downloading npm packages.

Any of the packages supported by the [npm](https://www.npmjs.org/) tool can be downloaded using the **jx install** tool.
The difference is, that the most popular npm packages were packed into jx files and stored in JXcore repository.
You can download a package as a single jx file and it is ready to be `required()` as regular npm module.

## Command

### install

    > jx install [-g] name_of_the_package[@version]

Downloads npm package specified by `name_of_the_package`. If this package exists in JXcore repository, it will be downloaded as single .jx file.
Otherwise the nmp tool will be employed to install required package as a regular npm module.

`-g` - enables global installation, the same as npm tool does. In fact, this switch is available only for npm packages:
even if the package would be available on JXcore repository, it will not be installed when you use `-g` switch.
Instead the package will be installed from the npm repository.

Let’s discuss the following example: we would like to install the [express](https://github.com/visionmedia/express) module.

    > jx install express

Because this package is also available in JXcore repository, the command will just download a single file *express.jx* to the current folder.
Now this package is ready to be used inside of your application:

```js
var express = require("./express.jx");
```

In case if you would like to install different `version` of this package, you can type:

    > jx install express@3.3.3

Since this version (3.3.3) is not present in JXcore repository, the command will first download *npm.jx* file
(which is in fact the npm tool packed into the jx file), and will try to use it to download the required version of the express package.
Now you can use it like any other npm module:

```js
var express = require("express");
```

Please note that you don’t need to have the npm tool installed on your machine. JXcore will download it to the current local folder when needed. Once downloaded it can be used like npm.
For example, instead of:

    > npm install -g express

You can do the same by:

    > jx npm.jx install -g express

In fact, any npm’s command can be passed this way:

    > jx npm.jx ls
    > jx npm.jx uninstall express
    etc.