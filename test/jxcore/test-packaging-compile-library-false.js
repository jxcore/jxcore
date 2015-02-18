// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating jx package with `library` set to false and checks, if is able to require() it.
 Should not be able.
 */

// -------------   init part

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var dir = __dirname + path.sep + path.basename(__filename) + "-tmp-dir" + path.sep;
var js_file = dir + "my_library.js";
var jxp_file = dir + "my_package.jxp";
var jx_file = dir + "my_package.jx";
var finished = false;
var lib = undefined;

jx.rmdirSync(dir);
fs.mkdirSync(dir);
process.chdir(dir);

process.on("exit", function (code) {
  jx.rmdirSync(dir);
  assert.strictEqual(lib, undefined, "Library did load, but shouldnt!");
  assert.ok(finished, "Test did not finish!");
});

// -------------   exec part

var jxp = {
  "name": "my_package",
  "version": "1.0",
  "author": "",
  "description": "",
  "company": "",
  "website": "",
  "package": null,
  "startup": "my_library.js",
  "execute": null,
  "extract": false,
  "output": "my_package.jx",
  "files": [
    "my_library.js"
  ],
  "assets": [],
  "library": false // changed to false
};

fs.writeFileSync(js_file, "console.log('from inside module');");
fs.writeFileSync(jxp_file, JSON.stringify(jxp));

var child = cp.exec('"' + process.execPath + '" compile ' + path.basename(jxp_file), {timeout: 30000}, function (error, stdout, stderr) {

  assert.ok(fs.existsSync(jx_file), "Cannot find compiled package " + jx_file);


  // package should not be allowed to be required,
  // the try block  will not catch the error, since by design application exits
  try {
    lib = require(jx_file);
  } catch (ex) {
    // it's fine, there should be an error, since library should not be required
  }

  finished = true;
});

