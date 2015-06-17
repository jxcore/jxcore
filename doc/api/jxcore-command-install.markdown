# Module Installer

JXcore provides you with an easy and convenient way for downloading npm packages.

Any of the packages supported by the [npm](https://www.npmjs.org/) tool can be downloaded using the **jx install** tool
and the npm manager does not have to be installed in the system.

## Command

### install

    > jx install [-g] name_of_the_package[@version] [--autoremove "*.txt,dir1,dir2/file2.txt"]

Downloads npm package specified by `name_of_the_package`.

* `-g` - enables global installation, the same as npm tool does.

* `--autoremove` - allows to perform post-install files/dirs removal.
It searches the given names/masks recursively within installed module's directory and removes them.
This is particularly useful for apps which target mobile devices as it may result in significantly smaller package sizes.

    This option receives one or multiple values separated with comma. However you may use any other character by setting special environment variable [JX_ARG_SEP](jxcore-utils.html#jxargsep).


Let’s see the following example: we would like to install the [express](https://github.com/visionmedia/express) module.

    > jx install express

The command will just download the *express* package.
Now this package is ready to be used inside of your application:

```js
var express = require("express");
```

In case if you would like to install different `version` of this package, you can type:

    > jx install express@3.3.3

Please note that you don’t need to have the npm tool installed on your machine.

### npm

For any npm command you can call `jx npm [command]`, for example:

    > jx npm ls
    > jx npm version
    > jx npm uninstall express

Formerly this could be achieved with `jx install -[npm command]`.
Although this method is still available, it becomes **depreciated** as of now and may be removed in future releases:

    > jx install -ls
    > jx install -version
    > jx install -uninstall express
