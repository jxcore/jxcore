// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code spawns itself with mt-keep param and checks for spawn error
 */

if (process.isPackaged)
  return;

var path = require("path");
var fs = require("fs");
var cp = require("child_process");
var jx = require('jxtools');
var assert = jx.assert;

var str = 'ON EXIT WAS CALLED';
var finished = false;
var outFileName = process.cwd() + path.sep + "test-process-on-exit_out.txt";

// spawned process
if (process.argv.indexOf("spawned") !== -1) {

  if (process.subThread)
    process.release();

  process.on("exit", function (code) {
    // only once per process (either 0 or -1)
    if (process.threadId <= 0)
      fs.writeFileSync(outFileName, str);
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

  var exists = fs.existsSync(outFileName);
  assert.ok(exists, "Could not find output written to file " + outFileName);

  // checking results of mt
  var ret = fs.readFileSync(outFileName).toString();
  fs.unlinkSync(outFileName);

  assert.ok(ret.indexOf(str) !== -1, "process.on('exit') was not called for command: \n" + cmd);
});
