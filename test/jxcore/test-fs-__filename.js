// Copyright & License details are available under JXCORE_LICENSE file

// This test unit checks multiple fs methods against `__filename`
// under a jx/native package

var fs = require('fs');
var assert = require('assert');
var path = require('path');

var str_typeof = "typeof stat is not 'object'";
var str_instanceof = "stat is not instance of fs.Stats";
var engine = process.versions.v8 ? "V8" : "SM";
var errors = [];

var strictEqual = function (actual, expected, func_name, err) {
  if (actual !== expected)
    errors.push(engine + " - failed for fs." + func_name + "(): " + err);
};

var checkToString = function (stat, func_name) {
  try {
    var x = stat.toString();
  } catch (ex) {
    errors.push(engine + " - failed for fs." + func_name + "(): " + ": " + ex);
  }
};

var isError = function(err, func_name) {
  if (err) {
    errors.push(engine + ' - ' + func_name + "() error: " + err);
    return true;
  }
};

process.on('exit', function () {
  if (errors.length) {
    console.error(errors.join("\n"));
    throw new Error("There are errors.");
  }
});

// stat
fs.stat(__filename, function (err, stat) {
  if (isError(err, "stat")) return;
  strictEqual(typeof stat, "object", "stat", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "stat", str_instanceof);
  checkToString(stat, "stat");
});

// statSync
try {
  var stat = fs.statSync(__filename);
  strictEqual(typeof stat, "object", "statSync", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "statSync", str_instanceof);
  checkToString(stat, "statSync");
} catch (ex) {
  isError(ex, 'statSync');
}


// lstat
fs.lstat(__filename, function (err, stat) {
  if (isError(err, "lstat")) return;
  strictEqual(typeof stat, "object", "lstat", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "lstat", str_instanceof);
  checkToString(stat, "lstat");
});

// lstatSync
try {
  var stat = fs.lstatSync(__filename);
  strictEqual(typeof stat, "object", "lstatSync", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "lstatSync", str_instanceof);
  checkToString(stat, "lstatSync");
} catch (ex) {
  isError(ex, 'lstatSync');
}


// existsSync
var exists = fs.existsSync(__filename);
strictEqual(exists, true, 'existsSync', 'The SYNC fs.existsSync(__filename) returned false!');

// exists
fs.exists(__filename, function(exists) {
  strictEqual(exists, true, 'exists', 'The ASYNC fs.exists(__filename) returned false!');
});

// readFileSync
var buf = fs.readFileSync(__filename);
strictEqual(Buffer.isBuffer(buf), true, "readFileSync", "Result of readFileSync(__filename) should be Buffer");
strictEqual(buf.toString().indexOf('end_of_file') !== -1, true, "readFileSync", "Could not read `end_of_file` after readFileSync(__filename)");

// readFile
fs.readFile(__filename, function(err, buf) {
  if (isError(err, "readFile")) return;
  strictEqual(Buffer.isBuffer(buf), true, "readFile", "Result of readFile(__filename) should be Buffer");
  strictEqual(buf.toString().indexOf('end_of_file') !== -1, true, "readFile", "Could not read `end_of_file` after readFile(__filename)");
});



// leave the below comment, it takes part in the test:
// end_of_file