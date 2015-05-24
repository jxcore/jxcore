// Copyright & License details are available under JXCORE_LICENSE file


var assert = require('assert');
var os = require("os");
var info = jxcore.utils.OSInfo();


// from docs:
//var node_arch = [ 'arm', 'ia32', 'x64' ];

var pairs = {
  "is64": "x64",
  "is32": "ia32",
  "isARM": "arm"
};

// process.arch
for (var prop in pairs) {
  if (info[prop] && pairs.hasOwnProperty(prop)) {

    // on ARM do not test is32/is64
    if (info.isARM && prop !== "isARM")
      continue;

    assert.strictEqual(process.arch, pairs[prop],
      "Incompatibility of process.arch = `" + process.arch + "` and OSInfo()." + prop + " = `" + info[prop] + "`" +
      "The value of process.arch should be: `" + pairs[prop] + "`");
  }
}

// os.arch()
for (var prop in pairs) {
  if (info[prop] && pairs.hasOwnProperty(prop)) {

    // on ARM do not test is32/is64
    if (info.isARM && prop !== "isARM")
      continue;

    assert.strictEqual(os.arch(), pairs[prop],
      "Incompatibility of os.arch() = `" + os.arch() + "` and OSInfo()." + prop + " = `" + info[prop] + "`" +
      "The value of os.arch() should be: `" + pairs[prop] + "`");
  }
}