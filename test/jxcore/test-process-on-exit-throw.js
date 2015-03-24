// Copyright & License details are available under JXCORE_LICENSE file

/*
 This test checks if process.on('exit') is called after app crashes.
 It is performed inside child process to be able to catch the output.
 */

var fs = require("fs");
var path = require("path");

var isChild = process.argv[process.argv.length - 1] == "child";
var str_exitting = "Exiting with code";
var str_throw = "Error intended";
var outFileName = process.cwd() + path.sep + "test-process-on-exit-throw_out.txt";

if (isChild) {
  // this is the proper test case
  process.on('exit', function (code) {
      fs.writeFileSync(outFileName, str_exitting);
  });

  throw str_throw;// new Error(str_throw);
}


if (!isChild) {
  // this is main block for spawning a test process

  var cp = require("child_process");
  var assert = require('assert')
  var finished = false;

  var fname = process.IsEmbedded ? process.execPath : __filename.replace(".js.jx", ".jx");
  var child = cp.exec('"' + process.execPath + '" "' + fname + '" child', null, function (error, stdout, stderr) {

    var exists = fs.existsSync(outFileName);
    assert.ok(exists, "Could not find output written to file " + outFileName);

    // checking results of mt
    var ret = fs.readFileSync(outFileName).toString();
    fs.unlinkSync(outFileName);

    var str = "" + stdout + stderr;
    assert.ok(str.indexOf(str_throw) !== -1, "Cannot determine if child have thrown an error.");
    assert.ok(ret.indexOf(str_exitting) !== -1, "Child did not call on('exit') after throw.");

    finished = true;
  });


  process.on('exit', function (code) {
    assert.ok(finished, "Test did not finish!");
  });
}



