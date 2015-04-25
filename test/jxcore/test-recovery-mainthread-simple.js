// Copyright & License details are available under JXCORE_LICENSE file

/*
 This test checks if process.on('restart') is called after app crashes.
 It is performed inside child process to be able to catch the output.
 */


var isChild = process.argv[process.argv.length - 1] == "child";
var str_onrestart = "On restart called.";
var str_throw = "Error intended";


if (isChild) {

  // this is the proper test case
  process.on('restart', function (code) {
    console.log(str_onrestart);
  });

  throw str_throw;
}


if (!isChild) {
  // this is main block for spawning a test process

  var cp = require("child_process");
  var assert = require('assert')
  var finished = false;

  var fname = process.isPackaged ? process.execPath : __filename.replace(".js.jx", ".jx");
  var child = cp.exec('"' + process.execPath + '" "' + fname + '" child', null, function (error, stdout, stderr) {

    var str = "" + stdout + stderr;
    //console.log("error", error, "\n", "stdout " + stdout, "\n", "stderr " + stderr);

    assert.ok(str.indexOf(str_throw) !== -1, "Cannot determine if child have thrown an error.");
    assert.ok(str.indexOf(str_onrestart) !== -1, "Child did not call on('restart') after throw.");

    finished = true;
  });


  process.on('exit', function (code) {
    assert.ok(finished, "Test did not finish!");
  });
}



