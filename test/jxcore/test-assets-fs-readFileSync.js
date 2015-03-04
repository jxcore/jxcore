// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.readFileSync() on asset files of jx package
 */

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");


var checkFile = function (assetPath) {

  // we will compare file contents between file from read file system and file from jx assets

  // we run jx package from `_auto_jxcore_package` folder
  // but we want to compare the asset content with real file located at `jxcore`,
  // so we cut the "_package" part from dirname
  var dirname = exports.$JXP ? __dirname.replace("_package", '').replace("_native", '').replace("_auto_", "").replace("_single", "") : __dirname;
  var realPath = fs.realpathSync(dirname + path.sep + assetPath);
  var realBuf = fs.readFileSync(realPath);
  var realContents = realBuf.toString();

  var assetBuf = fs.readFileSync(assetPath);
  var assetContents = assetBuf.toString();
  assert.strictEqual(assetContents, realContents, "Content of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + assetContents + "`\nfrom file: \t`" + realContents + "`");
  assert.strictEqual(realBuf.length, assetBuf.length, "Bufer length of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + assetBuf.length + "`\nfrom file: \t`" + realBuf.length + "`");
};

checkFile("assets/file.txt");
checkFile("assets/assets1/file1.txt");
//binary
checkFile("assets/example.gif");