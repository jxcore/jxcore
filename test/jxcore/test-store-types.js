// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing different types of values: if saved value equals to read value
 */

var jx = require('jxtools');
var assert = jx.assert;
var store = jxcore.store;

var allowed = ["string", 55, 45.35, true, false, ["a", "b", "c"]];

var key = "ss" + process.threadId;

for (var a = 0, len = allowed.length; a < len; a++) {
  store.set(key, allowed[a]);

  var v = store.read(key);
  assert.strictEqual(v, allowed[a] + "", "Read() value for key should be " + allowed[a] + " and is " + v);
}

if (process.threadId !== -1)
  process.release();