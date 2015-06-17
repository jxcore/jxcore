// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing splitBySep() method of process.argv parsed by jxcore.utils.argv.parse()

var assert = require('assert');

var newArgv = function (arr) {
  process.argv = [process.execPath, __filename].concat(arr);
  return jxcore.utils.argv.parse({force: true});
};

var checkEqual = function (org, expected) {
  assert.strictEqual(org, expected, "Values not equal. This: `" + org + "` instead of `" + expected + "`");
};


// no value after arg name
var json = newArgv(["-forSplit"]);
checkEqual(json.forSplit.value, undefined);
checkEqual(json.forSplit.splitBySep(), null);

// an empty string
var json = newArgv(["-forSplit", ""]);
checkEqual(json.forSplit.value, "");
checkEqual(json.forSplit.splitBySep(), null);

// whitespace
var json = newArgv(["-forSplit", " \n"]);
checkEqual(json.forSplit.value, " \n");
checkEqual(json.forSplit.splitBySep(), null);

// one value
var json = newArgv(["-forSplit", "*.gz"]);
checkEqual(json.forSplit.splitBySep()[0], "*.gz");

// multiple values. comma is default sep
var json = newArgv(["-forSplit", "*.gz,*.txt,samples,,last"]);
checkEqual(json.forSplit.splitBySep()[0], "*.gz");
checkEqual(json.forSplit.splitBySep()[1], "*.txt");
checkEqual(json.forSplit.splitBySep()[2], "samples");
// an empty string is skipped
checkEqual(json.forSplit.splitBySep()[3], "last");



