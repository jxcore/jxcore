// Copyright & License details are available under JXCORE_LICENSE file


var sh = jxcore.store.shared;

var jx = require('jxtools');
var assert = jx.assert;

var key2 = "alo2" + process.threadId;
assert.strictEqual(sh.read(key2), undefined, "Value for " + key2 + " should not exist.");

ret = sh.setIfEqualsToOrNull(key2, 0, -1);
assert.strictEqual(ret, true, "Result of shared.setIfEqualsToOrNull('" + key2 + "') should be true, but is " + ret);
assert.strictEqual(sh.read(key2), "0", "Value for " + key2 + " should be = 0.");

// this value will not be assigned, since value already exists and is != -1 and is not null
ret = sh.setIfEqualsToOrNull(key2, 1, -1);
assert.strictEqual(ret, false, "Result of shared.setIfEqualsToOrNull('" + key2 + "') should be false, but is " + ret);
assert.strictEqual(sh.read(key2), "0", "Value for " + key2 + " should be = 0.");

if (process.threadId !== -1)
  process.release();