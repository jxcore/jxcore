// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating jx package with `library` set to false and checks, if is able to require() it.
 Should not be able.
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
var createPackages = function (extract_what, native, cb) {

  process.chdir(__dirname);

  var cmd = "JX_BINARY package _asset_file.js myTestPkg -add assets --extract-where=./";
  if (extract_what)
    cmd += ' --extract-what "' + extract_what + '"';

  if (native)
    cmd += " -native";

  var batch = jx.saveBatchFile(cmd);
  cp.exec(batch, function(err, stdout, stderr) {
    if (err)
      console.log(err + "\n" + stdout + "\n" + stderr);

    assert.ifError(err, "Error during packaging");
    cb();
  });
};


var test = function (extract_what, filesShouldExist, filesShouldNotExists, native) {

  createPackages(extract_what, native, function() {
    var sid = native ? "NATIVE package:" : "JX package:";
    var isWindows = process.platform === "win32";

    jx.rmdirSync(dir);
    fs.mkdirSync(dir);

    var binaryBaseName = !native ? "myTestPkg.jx" : isWindows ? "myTestPkg.exe" : "myTestPkg";
    fs.renameSync(path.join(process.cwd(), binaryBaseName), path.join(dir, binaryBaseName));

    process.chdir(dir);

    if (!native)
      var ret = jxcore.utils.cmdSync('"' + process.execPath + '" ' + binaryBaseName);
    else
      var ret = jxcore.utils.cmdSync((isWindows ? "": "./") + binaryBaseName);

    if (ret.exitCode)
      console.log(ret.out);

    assert.strictEqual(ret.exitCode, 0, "Error during executing the package for extraction.");

    for (var o in filesShouldExist) {
      var file = path.join(process.cwd(), filesShouldExist[o]);
      assert.ok(fs.existsSync(file), sid + " file was NOT extracted, but should be:\n" + file + "\n\n");
    }

    for (var o in filesShouldNotExists) {
      var file = path.join(process.cwd(), filesShouldNotExists[o]);
      assert.ok(!fs.existsSync(file), sid + " file was extracted, but should NOT be:\n" + file + "\n\n");
    }

    if (!native)
      test(extract_what, filesShouldExist, filesShouldNotExists, true);
    else
      next();
  });
};


// -------------   exec part

var definitions = [
    {
      extract_what: "*.txt",
      filesShouldExist: ["assets/assets1/file1.txt"],
      filesShouldNotExists: ["assets/assets1/nubisa.png"]
    },
    {
      extract_what: "*.txt,*.png,example.gif",
      filesShouldExist: ["assets/assets1/file1.txt", "assets/assets1/nubisa.png", "assets/example.gif"],
      filesShouldNotExists: ["assets/package.json" ]
    }
];

var next = function() {
  var item = definitions.shift();
  if (item)
    test(item.extract_what, item.filesShouldExist, item.filesShouldNotExists);
};

next();
