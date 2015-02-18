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
  "extract": true,  // changed to true
  "output": "my_package.jx",
  "files": [
    "my_library.js"
  ],
  "assets": [
    "asset1.txt", "asset2.css", "asset3.json"
  ]
};

// writing some assets
for (var a = 0, len = jxp.assets.length; a < len; a++) {
  fs.writeFileSync(dir + jxp.assets[a], jxp[a]);
}

fs.writeFileSync(js_file, "console.log('from inside module');");
fs.writeFileSync(jxp_file, JSON.stringify(jxp, null, 4));

// compiling the jxp file
var child = cp.exec('"' + process.execPath + '" compile ' + path.basename(jxp_file), {timeout: 30000}, function (error, stdout, stderr) {

  assert.ok(fs.existsSync(jx_file), "Cannot find compiled package " + jx_file);

  // we want to remove those files, then we'll extract package and check if they are extracted
  var to_remove = jxp.assets;
  to_remove.push(path.basename(js_file));

  for (var a = 0, len = to_remove.length; a < len; a++) {
    fs.unlinkSync(to_remove[a]);
  }

  // now lets run the package and see, if contents was extracted
  var child = cp.exec('"' + process.execPath + '" ' + path.basename(jx_file), {timeout: 30000}, function (error, stdout, stderr) {

    for (var a = 0, len = to_remove.length; a < len; a++) {
      var fname = dir + path.basename(jx_file, ".jx") + path.sep + to_remove[a];
      assert.ok(fs.existsSync(fname), "Cannot find extracted asset " + fname);
    }

    finished = true;
  });
});

