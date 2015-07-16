// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing jxcore.utils.argv.remove() on process.argv

var assert = require('assert');

var newArgv = function(arr) {
  process.argv = [ process.execPath, __filename].concat(arr);
};

var checkEqual = function(org, expected) {
  assert.strictEqual(org, expected, "Values not equal. This: `" + org + "` instead of `" + expected + "`");
};

// arg with value
newArgv([ "--test1=val1", "--test2",  "val2", "--test3 val3" ]);
var ret = jxcore.utils.argv.remove("--test2", true);
checkEqual(ret, true);
checkEqual(process.argv[2], "--test1=val1");
checkEqual(process.argv[3], "--test3 val3");

// arg without value
newArgv([ "--test1=val1", "--test2", "--test3 val3" ]);
var ret = jxcore.utils.argv.remove("--test2", false);
checkEqual(ret, true);
checkEqual(process.argv[2], "--test1=val1");
checkEqual(process.argv[3], "--test3 val3");

// arg not found
newArgv([ "--test1=val1", "--test2", "--test3 val3" ]);
var org = process.argv.slice(0); // copy
var ret = jxcore.utils.argv.remove("--test7", false);
checkEqual(ret, false);
checkEqual(process.argv.toString(), org.toString());

