// Copyright & License details are available under JXCORE_LICENSE file

// The test checks for async `readFile` extension for fs module, when called with options.
// Also if it does not break fs.readFile on real fs/packaged file.

// async support for fs extensions is disabled for now
return;

var fs = require("fs");
var path = require("path");
var assert = require("assert");
var dummy_file = "dummy_file.txt";
var dummy_file_contents = "contents of dummy file";
var finished = 0;

var ext = {
  readFile : function(path, options, cb) {
    var cb = cb || options;

    if (path === dummy_file) {

      // async operation
      setTimeout(function() {
        cb(false, dummy_file_contents);
      }, 500);
      return;
    }

    // true means error
    cb(true);
  }
};

fs.setExtension("extensions", ext);

process.on("exit", function(code) {
  var total = 3;
  if (exports.$JXP) total += 1;
  assert.strictEqual(finished, total, "Not all callback were called " + finished + " out of " + total);
});

// ========== test #1 for non existing file, known to extension
fs.readFile(dummy_file, { opt : true } , function(err, data) {
  finished++;
  assert.ok(!err, "#1. There should be no error: " + err);
  assert.strictEqual(data.toString(), dummy_file_contents,
    "#1. Contents does not match: " + data.toString() + " !== " + dummy_file_contents);
});

// ========== test #2 for non existing file, not known to extension
fs.readFile("really_dummy.txt", { opt : true } , function(err, data) {
  finished++;
  assert.ok(err, "#2. There should be an error, but there isn't:" + err);
  assert.ok(!data, "#2. There should not be any data: " + data);
});


// ========== test #3 for existing file
fs.readFile(path.join(__dirname, "testcfg.py"), { opt : true } , function(err, data) {
  finished++;
  assert.ok(!err, "#3. There should not be any error, but there is:" + err);

  var syncData = fs.readFileSync(path.join(__dirname, "testcfg.py")).toString();
  assert.strictEqual(data.toString(), syncData,
    "#3. Contents does not match: " + data.toString() + " !== " + syncData);
});


// inside a package
if (exports.$JXP || process.isPackaged) {

  // ========== test #4 for existing and packaged file
  fs.readFile(path.join(__dirname, "_asset_file.txt"), { opt : true } , function(err, data) {
    finished++;

    assert.ok(!err, "#4. There should not be any error, but there is:" + err);

    var syncData = fs.readFileSync(path.join(__dirname, "_asset_file.txt")).toString();
    assert.strictEqual(data.toString(), syncData,
      "#4. Contents does not match: " + data.toString() + " !== " + syncData);
  });

}


