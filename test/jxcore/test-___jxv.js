// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');

var expectedVersion = "v Beta-0.3.0.0";

var ret = jxcore.utils.cmdSync('"' + process.execPath + '" -jxv');
var color = jxcore.utils.console.setColor;
var str_insteadof = " instead of " + color(expectedVersion, "green");

assert.strictEqual(expectedVersion, ret.out.trim(),
  "Incorrect JXcore version (jx -jxv): "
  + color(ret.out.trim(), "yellow") + str_insteadof);
assert.strictEqual(expectedVersion, process.jxversion,
  "Incorrect JXcore version (process.jxversion): "
  + color(process.jxversion, "yellow") + str_insteadof);