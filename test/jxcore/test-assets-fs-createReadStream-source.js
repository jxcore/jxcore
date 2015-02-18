// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.createReadStream() on source files of jx package
 */

if (!exports.$JXP)
  return;

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");

var counters = {};


var checkFile = function (assetPath) {

  counters[assetPath] = {};

  var assetContents = "";
  var p = path.join(__dirname, assetPath);
  var stream = fs.createReadStream(p);

  stream.on('data', function (chunk) {
    if (chunk !== false) {
      counters[assetPath]["ondata"] = true;
      assetContents += chunk.toString();
//            console.log("File was read: \n" + assetContents);
      console.log("File was read!");
    }
  });
};

var arr = [
  "assets/subfolder1/module1.js",
  "assets/subfolder2/node_path_module/index.js"
];

for (var id in arr) {
  checkFile(arr[id]);
}

process.on('exit', function () {
  for (var id in arr) {
    var file = arr[id];
    assert.ok(!counters[file]["ondata"], "stream.on('data') event was fired for " + file + " but should not!");
    assert.ok(!counters[file]["onend"], "stream.on('end') event was fired for " + file + " but should not!");
  }
});