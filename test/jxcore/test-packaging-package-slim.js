// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating a jx package with -slim parameter.
 Then we check if slimmed folder is present in package, or not (by calling require())
 */

if (process.isPackaged || exports.$JXP)
  return;

// -------------   init part

var jx = require('jxtools');
var assert = jx.assert;

var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var dir = __dirname + path.sep + path.basename(__filename) + "-tmp-dir" + path.sep;
var js_file = dir + "some_library.js";
var jx_file = dir + "my_package.jx";
var finished = false;

jx.rmdirSync(dir);
fs.mkdirSync(dir);
process.chdir(dir);

process.on("exit", function (code) {
  jx.rmdirSync(dir);
  assert.ok(finished, "Test did not finish!");
});

// -------------   exec part


// contents of a file will be empty
fs.writeFileSync(js_file, "");

// writing some assets
var assets = [
  "asset1.txt", "asset2.css", "asset3.xml"
];

var subFolder_name = "folder1";
var subFolder = dir + subFolder_name + path.sep;
fs.mkdirSync(subFolder);

var subFolder_slimed_name = "folder_slimed";
var subFolder_slimed = dir + subFolder_slimed_name + path.sep;
fs.mkdirSync(subFolder_slimed);

var wrap = function (txt) {
  return "exports.value = '" + txt + "';"
};

// writing some assets
for (var a = 0, len = assets.length; a < len; a++) {
  fs.writeFileSync(dir + assets[a], wrap(dir + assets[a]));
  fs.writeFileSync(subFolder + assets[a], wrap(subFolder + assets[a]));
  fs.writeFileSync(subFolder_slimed + assets[a], wrap(subFolder_slimed + assets[a]));
}

// we create a package first
var child = cp.exec('"' + process.execPath + '" package ' + path.basename(js_file) + " my_package -slim " + subFolder_slimed_name, {timeout: 30000}, function (error, stdout, stderr) {

  assert.ok(fs.existsSync(jx_file), "Cannot find compiled package " + jx_file);

  jx.rmdirSync(subFolder);
  jx.rmdirSync(subFolder_slimed);
  for (var a = 0, len = assets.length; a < len; a++) {
    fs.unlinkSync(dir + assets[a], wrap(dir + assets[a]));
   }

  var lib = require(jx_file);

  for (var a = 0, len = assets.length; a < len; a++) {
    var expected = wrap(dir + assets[a]);
    var contents = fs.readFileSync(assets[a]).toString();
    assert.strictEqual(contents, expected, "The asset: `" + assets[a] + "` contains wrong value: " + contents + "\n, but should contain: " + expected);

    // this should be present in $JXP.assets[]
    var str = subFolder_name + path.sep + assets[a];
    var expected = wrap(subFolder + assets[a]);
    var contents = fs.readFileSync(str).toString();
    assert.strictEqual(contents, expected, "The asset: `" + str + "` contains wrong value: " + contents + "\n, but should contain: " + expected);

    // this should NOT be present in $JXP.assets[]
    var str = subFolder_slimed_name + path.sep + assets[a];
    try {
      var contents = fs.readFileSync(str).toString();
      jx.throwMT("The asset should not be accessible: " + str);
    } catch (ex) {
      // exception is expected
    }
  }

  finished = true;
});
