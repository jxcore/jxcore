// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing fs.readFile() on asset files of jx package
 */

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");

var checkFile = function (assetPath, isBinary) {

  // we will compare file contents between file from read file system and file from jx assets

  // we run jx package from `_auto_jxcore_package` folder
  // but we want to compare the asset content with real file located at `jxcore`,
  // so we cut the "_package" part from dirname
  var dirname = exports.$JXP ? __dirname.replace("_package", '').replace("_native", '').replace("_auto_", "").replace("_single", "") : __dirname;
  var realPath = fs.realpathSync(dirname + path.sep + assetPath);
  var realStats = fs.statSync(realPath);
  var realBuf = fs.readFileSync(realPath);
  var realContents = realBuf.toString();

  fs.readFile(assetPath, function (err, data) {
//        console.log(data);
    if (err) {
      console.error("Cannot readFile() on asset: " + assetPath + "\n" + err);
    } else {
      var assetContents = data.toString();
      assert.strictEqual(assetContents, realContents, "Content of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + assetContents + "`\nfrom file: \t`" + realContents + "`");
      assert.strictEqual(realBuf.length, data.length, "Bufer length of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + data.length + "`\nfrom file: \t`" + realBuf.length + "`");
      assert.strictEqual(realContents.length, assetContents.length, "String length of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + assetContents.length + "`\nfrom file: \t`" + realContents.length + "`");

//            console.log("" + realContents.length + ", "  + assetContents.length);

      //saving to disk and comparing size of the file
      var tmpFileName = __dirname + path.sep + path.basename(assetPath);

      // writing as buf
      fs.writeFileSync(tmpFileName, data);
      var assetStats = fs.statSync(tmpFileName);
      assert.strictEqual(realStats.size, assetStats.size, "After writing file as Buffer, stat.size of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + assetStats.size + "`\nfrom file: \t`" + realStats.size + "`");

      if (!isBinary) {
        // writing as string
        fs.writeFileSync(tmpFileName, assetContents);
        var assetStats = fs.statSync(tmpFileName);
        fs.unlinkSync(tmpFileName);
        assert.strictEqual(realStats.size, assetStats.size, "After writing file as String, stat.size of asset file and real file (" + assetPath + ") are not equal.\nfrom asset: \t`" + assetStats.size + "`\nfrom file: \t`" + realStats.size + "`");
      } else {
        fs.unlinkSync(tmpFileName);
      }
    }
  });
};

checkFile("assets/file.txt");
checkFile("assets/assets1/file1.txt");
// binary file:
checkFile("assets/example.gif", true);
checkFile("assets/assets1/nubisa.png", true);
