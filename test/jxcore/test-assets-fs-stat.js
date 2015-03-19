// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.stat() on asset files of jx package
 */

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");


var checkFile = function (assetPath) {

  // we will compare file stats between file from read file system and file from jx assets

  // we run jx package from `_auto_jxcore_package` folder
  // but we want to compare the asset content with real file located at `jxcore`,
  // so we cut the "_package" part from dirname
  var dirname = exports.$JXP ? __dirname.replace("_package", '').replace("_native", '').replace("_auto_", "").replace("_single", "") : __dirname;
  var realPath = fs.realpathSync(dirname + path.sep + assetPath);
  var realStats = fs.statSync(realPath);

  fs.stat(assetPath, function (err, assetStats) {
    assert.ok(realStats, "Stats of real file " + realPath + " are: " + realStats);
    assert.ok(assetStats, "Stats of asset file " + assetPath + " are: " + assetStats);

    assert.ok(assetStats instanceof fs.Stats, "asset stats is not instanceof fs.Stats");
    assert.ok(realStats instanceof fs.Stats, "real stats is not instanceof fs.Stats");

    assert.strictEqual(typeof assetStats, "object", "`typeof stats` for asset is '" + (typeof assetStats) + "' instead of 'object'");
    assert.strictEqual(typeof realStats, "object", "`typeof stats` for real file is '" + (typeof realStats) + "' instead of 'object'");
  });
};

checkFile("assets/file.txt");
checkFile("assets/assets1/file1.txt");
checkFile("assets/subfolder1/module1.js");