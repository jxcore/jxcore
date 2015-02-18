// Copyright & License details are available under JXCORE_LICENSE file


var sh = jxcore.store.shared;

var jx = require('jxtools');
var assert = jx.assert;

var key = "alo11" + process.threadId;
var ret = sh.read(key);
assert.strictEqual(ret, undefined, "Value for " + key + " should not exist.");

var ret = sh.setIfNotExists(key, 0);
assert.strictEqual(ret, true, "Result of shared.setIfNotExists('" + key + "') should be true, but is " + ret);
assert.strictEqual(sh.read(key), "0", "Value for " + key + " should be = 0.");

// this value will not be assigned, since value already exists
ret = sh.setIfNotExists(key, 1);
assert.strictEqual(ret, false, "Result of shared.setIfNotExists('" + key + "') should be false, but is " + ret);
assert.strictEqual(sh.read(key), "0", "Value for " + key + " should be = 0.");

if (process.threadId !== -1)
  process.release();