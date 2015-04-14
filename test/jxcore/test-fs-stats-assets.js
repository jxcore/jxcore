// Copyright & License details are available under JXCORE_LICENSE file

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

process.on('exit', function () {
  if (errors.length) {
    console.error(errors.join("\n"));
    throw new Error("There are errors.");
  }
});

var filename = path.join(__dirname, "_asset_file.txt");

// stat
fs.stat(filename, function (err, stat) {
  strictEqual(typeof stat, "object", "stat", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "stat", str_instanceof);
  checkToString(stat, "stat");
});

// statSync
var stat = fs.statSync(filename);
strictEqual(typeof stat, "object", "statSync", str_typeof);
strictEqual(stat instanceof fs.Stats, true, "statSync", str_instanceof);
checkToString(stat, "statSync");


// lstat
fs.lstat(filename, function (err, stat) {
  strictEqual(typeof stat, "object", "lstat", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "lstat", str_instanceof);
  checkToString(stat, "lstat");
});

// lstatSync
var stat = fs.lstatSync(filename);
strictEqual(typeof stat, "object", "lstatSync", str_typeof);
strictEqual(stat instanceof fs.Stats, true, "lstatSync", str_instanceof);
checkToString(stat, "lstatSync");


var fd = fs.openSync(filename, "r");

// fstat
fs.fstat(fd, function (err, stat) {
  strictEqual(typeof stat, "object", "fstat", str_typeof);
  strictEqual(stat instanceof fs.Stats, true, "fstat", str_instanceof);
  checkToString(stat, "fstat");
});

// fstatSync
var stat = fs.fstatSync(fd);
strictEqual(typeof stat, "object", "fstatSync", str_typeof);
strictEqual(stat instanceof fs.Stats, true, "fstatSync", str_instanceof);
checkToString(stat, "fstatSync");