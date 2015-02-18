// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.readFile() on source file of jx package
 */

if (!exports.$JXP)
  return;

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");
var ok = false;


var checkFile = function (assetPath) {

  fs.readFile(assetPath, function (err, data) {

    if (err) {
      ok = true;
    }
  });
};

checkFile("assets/subfolder1/module1.js", true);

process.on('exit', function (code) {
  assert.ok(ok, "The source file was read successfully, but should not be!")
});