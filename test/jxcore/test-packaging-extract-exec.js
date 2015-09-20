// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating extractable jx package and test execution of the package
 */

// -------------   init part

if (process.isPackaged || exports.$JXP)
  return;

var jx = require('jxtools');
var assert = jx.assert;

var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var dir = path.join(__dirname, path.basename(__filename) + "-tmp-dir");

jx.rmdirSync(dir);
fs.mkdirSync(dir);
process.chdir(__dirname);

process.on("exit", function (code) {
  jx.rmdirSync(dir);
  jx.rmfilesSync(path.join(__dirname, "myTestPkg.jxp"));
});

// -------------   functions

// creates a package by applying --extract-what argument
var createPackages = function (basename, cb) {

  process.chdir(__dirname);

  var cmd = "JX_BINARY package assets/test-packaging-extract-exec/" + basename + " myTestPkg -add './assets/test-packaging-extract-exec' --extract";

  var batch = jx.saveBatchFile(cmd);
  cp.exec(batch, function(err, stdout, stderr) {
    if (err)
      console.log(err + "\n" + stdout + "\n" + stderr);

    assert.ifError(err, "Error during packaging");
    cb();
  });
};


var test = function () {

  var bn = process.argv[2] || "sync.js";
  createPackages(bn, function() {

    jx.rmdirSync(dir);
    fs.mkdirSync(dir);

    var binaryBaseName = "myTestPkg.jx";
    fs.renameSync(path.join(process.cwd(), binaryBaseName), path.join(dir, binaryBaseName));

    process.chdir(dir);

    var arg = "test1234";
    cp.exec('"' + process.execPath + '" ' + binaryBaseName + ' ' + arg, {timeout: 30000}, function (error, stdout, stderr) {

      var str = "Error during executing the package (arg = '" + bn + "'). ";
      assert.ifError(error, str + error);
      assert.strictEqual(stderr + "", "", str + "stderr:\n\n" + stderr);
      assert.ok((stdout + "").indexOf(arg) !== -1, str + "The child's output should contain '" + arg + "':\n\n" + stdout);
    });
  });
};


// -------------   exec part

test();
