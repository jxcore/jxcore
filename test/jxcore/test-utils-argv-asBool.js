// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing asBool() of jxcore.utils.argv.parse() item
// and jxcore.utils.argv.getBoolValue()

var assert = require('assert');
var last = {};

process.on('exit', function (code) {
  if (code) {
    jxcore.utils.console.warn("Failed for :\n", last.arg);
    jxcore.utils.console.warn(JSON.stringify(last.parsed, null, 4), "yellow");
  }
});

var newArgv = function (arr) {
  process.argv = [process.execPath, __filename].concat(arr);
};

var check = function (arg, expectedValue) {
  newArgv(arg);
  last.arg = arg;
  last.parsed = jxcore.utils.argv.parse({force: true});

  var b = last.parsed["boolArg"];
  var gb = jxcore.utils.argv.getBoolValue("boolArg");

  var expectsBool = expectedValue === true || expectedValue === false;
  if (expectsBool) {
    assert.strictEqual(b.isBool, expectsBool, "The arg should return isBool = " + expectsBool);
    assert.strictEqual(gb, expectedValue, "getBoolValue() should return " + expectedValue + " instead of " + gb);
  } else {
    assert.strictEqual(b.isBool, expectedValue, "The arg.isBool should be undefined instead of = " + typeof b.isBool);
    // returns true as long as --boolArg is present in the args
    assert.strictEqual(gb, true, "getBoolValue() should return true instead of " + gb);
  }

  assert.strictEqual(b.asBool, expectedValue, "Values not equal: `" + b.asBool + "` instead of `" + expectedValue + "`");
};

var trueValues = ["1", "yes", "true"];
for (var o in trueValues) {
  var v = trueValues[o];
  check(["--boolArg=" + v], true);
  check(["--boolArg:" + v], true);
  check(["--boolArg", v], true);
}

var falseValues = ["0", "no", "false"];
for (var o in falseValues) {
  var v = falseValues[o];
  check(["--boolArg=" + v], false);
  check(["--boolArg:" + v], false);
  check(["--boolArg", v], false);
}

var nonBoolValues = [ "1.1", "text", "0.2", ""];
for (var o in nonBoolValues) {
  var v = nonBoolValues[o];
  check(["--boolArg=" + v], undefined);
  check(["--boolArg:" + v], undefined);
  check(["--boolArg", v], undefined);
}