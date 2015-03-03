// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code spawns itself with mt/mt-keep param (process.argv[2) and checks for spawn error
 */

if (process.IsEmbedded)
  return;

// spawned process
if (process.argv.indexOf("spawned") !== -1) {
  // do nothing, exit naturally
  if (process.subThread)
    process.release();
  return;
}

// main process

var cp = require("child_process");
var jx = require('jxtools');
var assert = require('assert');

process.on("exit", function (code) {
  assert.ok(finished, "Test did not finish!");
});


var arg = process.argv[2] || "";

// spawning itself
var cmd = '"' + process.execPath + '" ' + arg + " " + __filename.replace(".js.jx", ".jx") + ' spawned';
var child = cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {
  finished = true;

  if (error) {
    console.error("Error while executing " + arg + " app with command:\n" + cmd);
    console.log("error: " + error);
    console.log("stdout: " + stdout);
    console.log("stderr: " + stderr);
    process.exit(1);
  }
});
