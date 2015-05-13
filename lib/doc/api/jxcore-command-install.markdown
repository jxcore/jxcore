# Module Installer

JXcore provides you with an easy and convenient way for downloading npm packages.

Any of the packages supported by the [npm](https://www.npmjs.org/) tool can be downloaded using the **jx install** tool
and the npm manager does not have to be installed in the system.

## Command

### install

    > jx install [-g] name_of_the_package[@version]

Downloads npm package specified by `name_of_the_package`.

`-g` - enables global installation, the same as npm tool does.

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

For other `npm commands` you can call `jx install -[npm command]`

for example;

    > jx install -ls
    > jx install
    > jx install --production

