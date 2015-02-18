// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code tests if some of the evaluated values in a subthread are the same as in the mainthread.
 Code spawns itself in mt-keep mode and gets results from files, since we cannot rely on stdout.
 */


if (process.IsEmbedded)
  return;

var path = require("path");
var fs = require("fs");
var cp = require("child_process");
var assert = require('assert');
var clog = jxcore.utils.console.log;

var finished = false;


var mted = process.argv[1].substr(0, 2) === "mt";
var mtparam = "mt-keep:2";
var finished = false;

process.on("exit", function (code) {
  assert.ok(finished, "Test did not finish!");
});


var output = JSON.stringify(process.execPath);
var propName = "process.execPath";

// in order to compare process.argv, we remove argv[0] and argv[1] (mt/mt-keep)
if (propName === "process.argv") {

  var copy = process.argv.slice(0);
  if (mted) {
    // removing "mt" or "mt:keep" from process.argv[1]
    copy.splice(0, 2);
  } else {
    // also removing from test argv[0], since from mt it contains full path
    // and in ST it can be just 'jx'
    copy.splice(0, 1);
  }
  output = JSON.stringify(copy);
}


var outFileName = process.cwd() + path.sep + "test-mt-values-tmp-" + process.threadId + ".txt";

if (mted) {
  // this block runs with jx mt
  // saves evaluation results into the files

  if (process.threadId == 0) {
    fs.writeFileSync(outFileName, output);
  }

  finished = true;

  // forcing faster exit than naturally
  setTimeout(process.exit, 10);

  return;
}


// the rest of the test runs in main thread

// now evaluating inside mt-keep and comparing with ST evaluations
// we pass all process.argv except [0]
var cmd = '"' + process.execPath + '" ' + mtparam + ' ' + process.argv.slice(1).join(" ");

var fname = process.cwd() + path.sep + "test-mt-values-tmp-" + 0 + ".txt";
if (fs.existsSync(fname)) {
  fs.unlinkSync(fname);
}


var child = cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {

//    assert.ok(!error, "Error from child process: " + cmd + "\n" + stdout + stderr);

  var exists = fs.existsSync(fname);
  assert.ok(exists, "Could not find output written to file " + fname);

  // checking results of mt
  var ret = fs.readFileSync(fname).toString();
  fs.unlinkSync(fname);

  var str = propName;
  if (str == "process.argv") {
    str += " (without item [0])";
  }

  clog("main thread:", "green");
  clog("\t", output);

  clog("MT-KEEP:", "magenta");
  clog("\t", ret);

  assert.strictEqual(output, ret, "Values are not equal: process.execPath");
  finished = true;

  // forcing faster exit than naturally
  setTimeout(process.exit, 10);

});


