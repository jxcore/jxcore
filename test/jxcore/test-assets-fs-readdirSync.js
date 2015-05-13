// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.readdirSync() on asset files of jx package
 */


// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");


var checkFile = function (assetPath) {

  // we will compare file contents between file read from file system and file read from jx assets

  // we run jx package from `_auto_jxcore_package` folder
  // but we want to compare the asset content with real file located at `jxcore`,
  // so we cut the "_package" part from dirname
  var dirname = exports.$JXP ? __dirname.replace("_package", '').replace("_native", '').replace("_auto_", "").replace("_single", "") : __dirname;
  var realPath = fs.realpathSync(dirname + path.sep + assetPath);
  var realContents = fs.readdirSync(realPath);

  var assetsDirContents = fs.readdirSync(assetPath);


  for (var id in realContents) {
    var item = realContents[id];
    if (assetsDirContents.indexOf(item) === -1) {
      console.error("Did not find " + item + " in readdir('" + assetPath + "')");
    }
  }

  assert.strictEqual(realContents.length, assetsDirContents.length, "Error while testing readdirSync() on assets path: " + assetPath + "\Incorrect array lenght.\nfrom assets dir: \t`" + assetsDirContents.length + "`\nfrom disk dir: \t`" + realContents.length + "`");
};

checkFile("assets");
checkFile("assets/");
checkFile("assets/assets1");
checkFile("assets/assets1/");
checkFile("assets/subfolder1");
checkFile("assets/subfolder1/");