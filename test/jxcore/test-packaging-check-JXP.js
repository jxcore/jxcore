// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing accessibility of $JXP object of JX package
 */


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

var unicodeStrings = ["норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ",
  "text with slashes \ / \\ //"];

var sid = unicodeStrings.join("\n");

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
for (var a = 0; a < 10; a++) {
  fs.writeFileSync(dir + "asset" + a + ".txt", sid + a);
}


var child = cp.exec('"' + process.execPath + '" package ' + path.basename(js_file) + " my_package", {timeout: 30000}, function (error, stdout, stderr) {

  assert.ok(fs.existsSync(jx_file), "Cannot find compiled package " + jx_file);

  // removing js_file, since we don't need it any more
  fs.unlinkSync(js_file);

  for (var a = 0; a < 10; a++) {
    fs.unlinkSync(dir + "asset" + a + ".txt");
  }

  var lib = require(jx_file);

  // accessing JXP
  var jxp = null;
  try {
    jxp = lib.$JXP;
  } catch (ex) {
    jx.throwMT("Cannot access $JXP object: " + ex);
  }

  assert.strictEqual(lib.$JXP.name, "my_package");

  for (var a = 0; a < 10; a++) {
    var src = fs.readFileSync("./asset" + a + ".txt").toString();
    var expectedValue = sid + a;
    assert.strictEqual(src, expectedValue, "The asset: `asset" + a + ".txt` contains wrong value.");
  }

  finished = true;
});

