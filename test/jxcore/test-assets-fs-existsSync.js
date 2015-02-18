// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.existsSync() on asset files of jx package
 */

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");

var arr = [
  "_asset_file.txt",
  "assets/file.txt",
  "assets/assets1/file1.txt",
  "assets/subfolder1/module1.js",
];

for (var id in arr) {
  assert.ok(fs.existsSync(arr[id]), "The exists() method for asset file does not work: " + arr[id]);
}