node-uwp (JXcore embedded version)
==========

Enables Universal Windows Platform (UWP) API access for Node.js (Chakra build)
on Windows 10.

Example
-------

```javascript
var uwp = require('uwp');
uwp.projectNamespace("Windows");

Windows.Storage.KnownFolders.documentsLibrary.createFileAsync(
  "sample.dat", Windows.Storage.CreationCollisionOption.replaceExisting)
  .done(
    function (file) {
      console.log("ok");
      uwp.close(); // all async operations are completed, release uwp
    },
    function (error) {
      console.error("error", error);
      uwp.close(); // all async operations are completed, release uwp
    }
);
```

Installation
------------

### Prerequisites

 * Windows 10
 * Visual Studio 2015 (RC or later)
 * [Node.js build with Chakra on Windows](https://github.com/Microsoft/node/blob/ch0.12/README.md#windows_with_chakra)

Currently building node.js native addon modules with Chakra depends
on Node.js build. See [how to build and install native addon modules](https://github.com/Microsoft/node/blob/ch0.12/README.md#build_native_addon_modules_with_chakra).

```sh
node.exe [node_repo]\deps\npm\bin\npm-cli.js install uwp --nodedir=[node_repo]
```

APIs
----

This package exports 2 functions.

### projectNamespace(name)

Project a UWP namespace of given name.

* **Note**: This function will keep Node process alive so that your app can
  continue to run and handle UWP async callbacks. You need to call
  [close()](#close) when UWP usage is completed.

<a name="close"></a>
### close()

Close all UWP handles used by this package. Call this when all UWP usage is
completed.
