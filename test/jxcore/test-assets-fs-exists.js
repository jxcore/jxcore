// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.exists() on asset files of jx package
 */

// assets are read properly by combining process.cwd();
process.chdir(__dirname);


var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");


var arr = [
  "assets/file.txt",
  "assets/assets1/file1.txt",
  "assets/subfolder1/module1.js",
];

var id = -1;

var next = function () {
  id++;
  if (id >= arr.length) return;

  fs.exists(arr[id], function (exists) {
    assert.ok(exists, "The exists() method for asset file does not work: " + arr[id]);

    next();
  });
};


next();


