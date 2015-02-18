// Copyright & License details are available under JXCORE_LICENSE file

var jx = require('jxtools');
var assert = jx.assert;

var mem = jxcore.store.shared;

mem.set("DATA", Date.now());
mem.expires("DATA", 100);

setTimeout(function () {
  assert.strictEqual(mem.read("DATA"), undefined, "Data should be expired!");

  if (process.subThread) {
    process.release();
  }
}, 250);
