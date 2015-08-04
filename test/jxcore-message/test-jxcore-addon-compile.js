// Copyright & License details are available under JXCORE_LICENSE file


var jxtools = require("jxtools");
var fs = require("fs");
var path = require("path");
var assert = require('assert');

jxtools.listenForSignals();

// ******** get test - git needs to be available
var ret = jxcore.utils.cmdSync("git --version");
if (ret.exitCode) {
  console.error("Test cannot proceed: git command not available.");
  process.exit(8);
}

var dir = path.join(__dirname, "jxcore-addon");

process.on('exit', function() {
  jxtools.rmdirSync(dir);
});


// ******** git clone
process.chdir(__dirname);
var cp = require("child_process");
cp.exec("git clone https://github.com/jxcore/jxcore-addon", function(err, stdout, stderr) {

  if (err) {
    console.error("Cannot clone github repository:", stdout + "", stderr + "");
    process.exit(8);
  }

  // ******** jx install
  process.chdir(dir);
  var ret = jxcore.utils.cmdSync('"' + process.execPath + '" install --unsafe-perm');
  if (ret.exitCode) {
    console.error("Error while compiling the addon:\n", ret.out);
    process.exit(8);
  }

// ******** addon test
  var ret = jxcore.utils.cmdSync('"' + process.execPath + '" test.js');
  if (ret.exitCode) {
    console.error("Error while executing addon's test.js:\n", ret.out);
    process.exit(8);
  }

  console.log(ret.out);
});

