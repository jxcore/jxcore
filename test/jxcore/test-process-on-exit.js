// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code spawns itself with mt-keep param and checks for spawn error
 */

if (process.IsEmbedded)
  return;

var path = require("path");
var fs = require("fs");
var cp = require("child_process");
var jx = require('jxtools');
var assert = jx.assert;

var str = 'ON EXIT WAS CALLED';
var finished = false;

// spawned process
if (process.argv.indexOf("spawned") !== -1) {

  if (process.subThread)
    process.release();

  process.on("exit", function (code) {
    console.log(str);
    jx.exitNowMT();
  });
  return;
}


// main process

process.on("exit", function (code) {
  assert.ok(finished, "Test did not finish!");
});


var arg = process.argv[2] || "";

// spawning itself
var cmd = '"' + process.execPath + '" ' + arg + " " + __filename.replace(".js.jx", ".jx") + ' spawned';
var child = cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {
  finished = true;
  assert.ok(stdout.toString().indexOf(str) !== -1, "process.on('exit') was not called for command: \n" + cmd);
});
