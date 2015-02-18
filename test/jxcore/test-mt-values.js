// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code tests if some of the evaluated values in a subthread are the same as in the mainthread.
 Code spawns itself in mt-keep mode and gets results from files, since we cannot rely on stdout.
 */

var path = require("path");
var fs = require("fs");
var cp = require("child_process");
var jx = require('jxtools');
var assert = jx.assert;

var mted = process.argv[1].substr(0, 2) === "mt";
var output = [];
var finished = false;

process.on("exit", function (code) {
  assert.ok(finished, "Test did not finish!");
});


var arr = [
  "__dirname",
  "__filename",
  "process.mainModule.filename",
  "process.argv",
  "process.cwd()",
  "process.execPath",
  "process.execArgv",
  "process.arch",
  "process.platform"
];

var arr2 = [
  __dirname,
  __filename,
  process.mainModule.filename,
  process.argv,
  process.cwd(),
  process.execPath,
  process.execArgv,
  process.arch,
  process.platform
];


// evaluating variables and pushing results to an array
for (var a = 0, len = arr.length; a < len; a++) {
  var propName = arr[a];
  var ev = JSON.parse(JSON.stringify(arr2[a]));

  if (propName === "process.argv") {
    if (ev[1].trim().substr(0, 2) == "mt") {
      // removing "mt" or "mt:keep" from process.argv[1]
      ev.splice(0, 2);
    } else {
      // also removing from test argv[0], since from mt it contains full path
      // and in ST it can be just 'jx'
      ev.splice(0, 1);
    }
  }

  //ev = util.inspect(ev, { showHidden: true, depth: null });
  output.push(JSON.stringify(ev));
}

var dir = process.cwd() + path.sep + "test-mt-values-tmp" + path.sep;

if (mted) {
  // this block runs with jx mt
  // saves evaluation results into the files
  // output dir is provided in last process.argv[] parameter

  assert.ok(fs.existsSync(dir), "Cannot find output dir: " + dir);

  for (var a = 0, len = arr.length; a < len; a++) {
    var fname = dir + a + "_" + process.threadId + ".txt";
    fs.writeFileSync(fname, output[a]);
  }

  finished = true;
  process.release();
  return;
}


// the rest of the test runs in main thread

jx.rmdirSync(dir);
fs.mkdirSync(dir);

// now evaluating inside mt-keep and comparing with ST evaluations
// we pass all process.argv except [0]
var cmd = '"' + process.execPath + '" mt-keep:2 ' + process.argv.slice(1).join(" ");

var child = cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {

  //assert.ok(!error, "Error from child process: " + cmd + "\n" + stdout + stderr);

  // checking results of mt
  for (var a = 0, len = arr.length; a < len; a++) {
    var fname = dir + a + "_" + 0 + ".txt";
    var ret = fs.readFileSync(fname).toString();

    var exists = fs.existsSync(fname);
    assert.ok(exists, "Could not find output written to file " + fname);
    if (exists) {
      var str = arr[a];
      if (str == "process.argv") {
        str += " (without item [0])";
      }
      assert.strictEqual(output[a], ret, "MT-KEEP: Value " + str + " is:\n" + ret + "\nbut should be:\n" + output[a] + "\n\n");
      finished = true;
    }
  }

  jx.rmdirSync(dir);
});


