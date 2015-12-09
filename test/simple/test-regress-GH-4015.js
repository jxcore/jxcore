// Copyright & License details are available under JXCORE_LICENSE file

if (process.env.CITEST) {
  console.error('Skipping test due to CITEST environmental variable.');
  process.exit();
}

var common = require('../common');
var assert = require('assert');
var exec = require('child_process').exec;

var cmd = process.execPath
        + ' '
        + common.fixturesDir
        + '/test-regress-GH-4015.js';

exec(cmd, function(err, stdout, stderr) {
  if(process.versions.v8)
    assert(/RangeError: Maximum call stack size exceeded/.test(stderr));
  else
    assert(err.signal == "SIGSEGV");
});
