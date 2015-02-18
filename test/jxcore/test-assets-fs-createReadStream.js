// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.createReadStream() on asset files of jx package
 */

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");

var counters = {};


var checkFile = function (assetPath) {

  counters[assetPath] = {};

  // we will compare file contents between file from read file system and file from jx assets

  // we run jx package from `_auto_jxcore_package` folder
  // but we want to compare the asset content with real file located at `jxcore`,
  // so we cut the "_package" part from dirname
  var dirname = exports.$JXP ? __dirname.replace(/_package$/g, '').replace("_auto_", "").replace("_single", "") : __dirname;
  var realPath = fs.realpathSync(dirname + path.sep + assetPath);
  var realContents = fs.readFileSync(realPath).toString();

  var assetContents = "";
  try {
    var stream = fs.createReadStream(assetPath);
  } catch (ex) {
    throw "Cannot fs.createReadStream('" + assetPath + "'):\n" + ex;
    return;
  }

  stream.on('data', function (chunk) {
    counters[assetPath]["ondata"] = true;
    assetContents += chunk.toString();
  });

  stream.on('end', function () {
    counters[assetPath]["onend"] = true;
    if (counters[assetPath]["ondata"] && assetPath.indexOf(".json") === -1) {
      assert.strictEqual(assetContents, realContents, "Content of asset file and real file are not equal.\nfrom asset: \t`" + assetContents + "`\nfrom file: \t`" + realContents + "`");
      assert.strictEqual(assetContents.length, realContents.length, "String length of contents of asset file and real file are not equal.\nfrom asset: \t`" + assetContents.length + "`\nfrom file: \t`" + realContents.length + "`");
    }
  });

  stream.on('error', function (err) {
    counters[assetPath]["onerror"] = true;
    console.log("cwd:", process.cwd());
    console.log("__dirname:", __dirname);
    console.error("err:", err);
    throw "Error while reading asset file with createReadStream(): " + assetPath;
  });
};

var arr = [
  "assets/file.txt",
  "assets/assets1/file1.txt",
  "assets/package.json",
  "assets/example.gif",
  "assets/subfolder1/module1.js",
  "_asset_file.js"  // any js file
];

for (var id in arr) {
  checkFile(arr[id]);
}

process.on('exit', function () {
  for (var id in arr) {
    var file = arr[id];
    assert.ok(counters[file]["ondata"], "stream.on('data') event was not fired for " + file);
  }
});

