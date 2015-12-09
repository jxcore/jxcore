// Copyright & License details are available under JXCORE_LICENSE file


var assert = require('assert');

// The tests below should throw an error, not abort the process...
assert.throws(function() { new Buffer(0x3fffffff + 1) }, RangeError);

if (process.versions.v8) {
  assert.throws(function() { new Int8Array(0x3fffffff + 1) }, RangeError);
  assert.throws(function() { new ArrayBuffer(0x3fffffff + 1) }, RangeError);
  assert.throws(function() { new Float64Array(0x7ffffff + 1) }, RangeError);
}

if (process.versions.sm) {
  // sm doesn't have 32 bit memory limitation
  // see: https://github.com/jxcore/jxcore/issues/241
  assert.throws(function() { new Int8Array(0x3fffffff * 2 + 1) }, InternalError);
  assert.throws(function() { new ArrayBuffer(0x3fffffff * 2 + 2) }, RangeError);
  assert.throws(function() { new Float64Array(0x7ffffff * 2 + 1) }, InternalError);
}
