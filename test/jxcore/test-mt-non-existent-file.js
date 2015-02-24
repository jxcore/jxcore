// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code spawns non-existing file mt-keep param and checks for spawn error
 */

var path = require("path");
var fs = require("fs");
var cp = require("child_process");
var jx = require('jxtools');
var assert = jx.assert;


var cmd = '"' + process.execPath + '" mt-keep _fake_file.js';

var child = cp.exec(cmd, {timeout: 1000}, function (error, stdout, stderr) {

  var str = "" + error + stdout + stderr;
  assert.strictEqual(str.indexOf('restarting thread'), -1, "Error while spawning mt-keep: " + cmd + "\n" + error + stdout + stderr);
});
