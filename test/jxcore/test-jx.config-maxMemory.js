// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing maxCPU parameter from jxcore.config.
 */

var jx = require('jxtools');
var assert = jx.assert;
var cp = require("child_process");
var path = require("path");


var cmd = '"' + process.execPath + '" ' + path.join(__dirname, "jx_config/maxMemory/test.js");
var child = cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {

  var isAlive = false;
  try {
    isAlive = !!process.kill(child.pid, 0);
  } catch (ex) {
  }

  assert.notStrictEqual(isAlive, true, "There should be an error. maxCPU not working.");

  // extra check for unix platforms
  if (process.platform !== "win32") {
    var str = "" + stdout + stderr;
    assert.ok(str.indexOf("reached beyond the pre-defined memory limits") !== -1, "There should be an error. maxMemory not working.")
  }
});

