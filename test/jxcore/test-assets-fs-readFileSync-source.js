// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.readFileSync() on asset files of jx package
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

  try {
    var assetContents = fs.readFileSync(assetPath).toString();
  } catch (ex) {
    ok = true;
  }
};

checkFile("assets/subfolder1/module1.js");

process.on('exit', function (code) {
  assert.ok(ok, "The source file was read successfully, but should not be!")
});