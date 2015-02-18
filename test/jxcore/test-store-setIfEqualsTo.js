// Copyright & License details are available under JXCORE_LICENSE file

/*
 This test unit verifies behaviour of shared.setIfEqualsTo()
 */


var sh = jxcore.store.shared;

var jx = require('jxtools');
var assert = jx.assert;

var key3 = "alo3" + process.threadId;
assert.strictEqual(sh.read(key3), undefined, "Value for " + key3 + " should not exist.");

// this value will not be assigned, since value does not exists so cannot be equal to -1
ret = sh.setIfEqualsTo(key3, 0, -1);
assert.strictEqual(ret, false, "Result of shared.setIfEqualsTo('" + key3 + "') should be false, but is " + ret);
assert.strictEqual(sh.read(key3), undefined, "Value for " + key3 + " should not exist.");

ret = sh.set(key3, -1);
assert.strictEqual(sh.read(key3), "-1", "Value for " + key3 + " should be = -1.");

// value 0 should be assigned
ret = sh.setIfEqualsTo(key3, 0, -1);
assert.strictEqual(ret, true, "Result of shared.setIfEqualsTo('" + key3 + "') should be true, but is " + ret);
assert.strictEqual(sh.read(key3), "0", "Value for " + key3 + " should be = 0.");

// this value will not be assigned, since value value != -1
ret = sh.setIfEqualsTo(key3, "some_value", -1);
assert.strictEqual(ret, false, "Result of shared.setIfEqualsTo('" + key3 + "') should be false, but is " + ret);
assert.strictEqual(sh.read(key3), "0", "Value for " + key3 + " should be = 0.");


if (process.threadId !== -1)
  process.release();