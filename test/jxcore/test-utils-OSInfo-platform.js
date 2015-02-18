// Copyright & License details are available under JXCORE_LICENSE file


var assert = require('assert');
var os = require('os');
var info = jxcore.utils.OSInfo();


// from docs:
// var node_platforms = [ 'darwin', 'freebsd', 'linux', 'sunos', 'win32'];

var pairs = {
  "isMac": "darwin",
  "isWindows": "win32",
  "isBSD": "freebsd",
  "isUbuntu": "linux",
  "isDebian": "linux",
  "isRH": "linux",
  "isSuse": "linux"
};

// process.platform
for (var prop in pairs) {
  if (info[prop]) {
    assert.strictEqual(process.platform, pairs[prop],
      "Incompatibility of process.platform = `" + process.platform + "` and OSInfo()." + prop + " = `" + info[prop] + "`" +
      "The value of process.platform should be: `" + pairs[prop] + "`");
  }
}

// os.platform()
for (var prop in pairs) {
  if (info[prop]) {
    assert.strictEqual(os.platform(), pairs[prop],
      "Incompatibility of os.platform() = `" + os.platform() + "` and OSInfo()." + prop + " = `" + info[prop] + "`," +
      "The value of os.platform() should be: `" + pairs[prop] + "`");
  }
}