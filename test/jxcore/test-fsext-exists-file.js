// Copyright & License details are available under JXCORE_LICENSE file

// The test checks for async `exists` extension for files.
// Also if it does not break fs.exists on real fs/packaged files.

var fs = require("fs");
var path = require("path");
var assert = require("assert");
var dummy_file = "dummy_file.txt";
var finished = 0;

var ext = {
  exists : function(path, cb) {
    jxcore.utils.console.log("called: ext exists", path, "yellow");
    cb(path === dummy_file ? true : "unknown");
  }
};

fs.setExtension("extensions", ext);

process.on("exit", function(code) {
  var total = 3;
  if (exports.$JXP) total += 1;
  assert.strictEqual(finished, total, "Not all callback were called " + finished + " out of " + total);
});

// ========== test #1 for non existing file, known to extension
fs.exists(dummy_file, function(exists) {
  finished++;
  assert.ok(exists, "#1. Extension method should return true instead of " + exists);
});

// ========== test #2 for non existing file, not known to extension
fs.exists("really_dummy.txt", function(exists) {
  finished++;
  assert.ok(exists === false, "#2. The result should be false instead of " + exists);
});

// ========== test #3 for existing file
fs.exists(path.join(__dirname, "testcfg.py"), function(exists) {
  finished++;
  assert.ok(exists, "#3. The result should be true instead of " + exists);
});

// inside a package
if (exports.$JXP || process.isPackaged) {

  // ========== test #4 for existing and packaged file
  fs.exists(path.join(__dirname, "_asset_file.txt"), function(exists) {
    finished++;
    assert.ok(exists, "#4. The result should be true instead of " + exists);
  });

}