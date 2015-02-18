// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing reading non existent key from the store
 */

var jx = require('jxtools');
var assert = jx.assert;

var key = "non_existing_key_" + process.threadId;
assert.strictEqual(jxcore.store.shared.read(key), undefined, "Value for " + key + " should not exist.");

if (process.threadId !== -1)
  process.release();