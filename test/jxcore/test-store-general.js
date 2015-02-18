// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code tests various things related to store.
 */

var jx = require('jxtools');
var assert = jx.assert;

var store = jxcore.store;
var shared = jxcore.store.shared;

// check if store/store.shared object exists
assert.notStrictEqual(store, undefined, "jxcore.store is not available");
assert.notStrictEqual(shared, undefined, "jxcore.store.shared is not available");

// test if we can write zero
require('assert').doesNotThrow(function () {
  // trying to write 0
  store.set("zero", 0.0);
  store.set("zero", 0);
});


// testing of exists()
assert.notStrictEqual(store.exists(22), undefined, "Key 22 should not exist");
store.set(22, "22");
assert.strictEqual(store.exists(22), true, "Key 22 should exist");

// store.shared
assert.notStrictEqual(shared.exists(22), undefined, "Key 22 should not exist in shared store");
shared.set(22, 55);
assert.strictEqual(shared.exists(22), true, "Key 22 should exist in shared store");

assert.equal(store.read(22), 22);
assert.equal(shared.read(22), 55);


// setIf...
assert.strictEqual(shared.setIfEqualsToOrNull("alo", 1, "null"), true, "Return from setIfEqualsToOrNull != true");
assert.strictEqual(shared.setIfEqualsToOrNull("alo", 1, "null"), false, "Return from setIfEqualsToOrNull != false");

assert.strictEqual(shared.setIfNotExists("alo1", 1), true, "Return from setIfNotExists != true");
assert.strictEqual(shared.setIfNotExists("alo1", 1), false, "Return from setIfNotExists != false");


if (process.threadId !== -1)
  process.release();