// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing maxCPU parameter from jxcore.config.
 */

var jx = require('jxtools');
var assert = jx.assert;
var cp = require("child_process");
var path = require("path");


var cmd = '"' + process.execPath + '" ' + path.join(__dirname, "jx_config/maxMemory/test.js");
cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {

  var str = "" + stdout + stderr;
  console.log(cmd,str);
  assert.ok(str.indexOf("reached beyond the pre-defined memory limits") !== -1, "There should be an error. maxMemory not working.")
});

