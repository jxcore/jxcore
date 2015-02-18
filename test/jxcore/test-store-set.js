// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit does the following with the standard store:
 1. stores cnt elements to the store
 2. reads them and verifies value
 3. test if values still exist (they should)
 4. get() or remove()
 5. again checks, if values exists (should not)
 */

var jx = require('jxtools');
var assert = jx.assert;

var store = jxcore.store;

var cnt = 10000;

for (var a = 0; a < cnt; a++) {
  store.set("test" + a, 100 + a);
}

// read()
for (var a = 0; a < cnt; a++) {
  var v = store.read("test" + a);
  assert.equal(v, 100 + a, "Read() value for key 'test" + a + "' is should be " + (100 + a) + " and is " + v);
}

// exists
for (var a = 0; a < cnt; a++) {
  var v = store.exists("test" + a);
  assert.strictEqual(v, true, "Value for key 'test" + a + "' does not exist");
}

// get() and remove()
for (var a = 0; a < cnt; a++) {
  if (a % 2 == 0) {
    var v = store.get("test" + a);
    assert.equal(v, 100 + a, "Read() value for key 'test" + a + "' is should be " + (100 + a) + " and is " + v);
  } else {
    store.remove("test" + a);
  }
}

// again exists
for (var a = 0; a < cnt; a++) {
  var v = store.exists("test" + a);
  assert.strictEqual(v, false, "Value for key 'test" + a + "' should not exist");
}

if (process.threadId !== -1)
  process.release();