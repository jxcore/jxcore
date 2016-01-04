// Copyright & License details are available under JXCORE_LICENSE file

/*
 * Tests to verify slice functionality of ArrayBuffer.
 * (http://www.khronos.org/registry/typedarray/specs/latest/#5)
 */
 
// Chakra, SpiderMonkey && v8 3.15+ implementation has native TypedArray support

var ver = process.versions;
if (ver.sm || ver.ch) return;
if (parseFloat(ver.v8)>3.14) return;

var common = require('../common');
var assert = require('assert');

var buffer = new ArrayBuffer(8);
var sub;
var i;

for (var i = 0; i < 8; i++) {
  buffer[i] = i;
}

// slice a copy of buffer
sub = buffer.slice(2, 6);

// make sure it copied correctly
assert.ok(sub instanceof ArrayBuffer);
assert.equal(sub.byteLength, 4);

for (i = 0; i < 4; i++) {
  assert.equal(sub[i], i+2);
}

// modifications to the new slice shouldn't affect the original
sub[0] = 999;

for (i = 0; i < 8; i++) {
  assert.equal(buffer[i], i);
}

// test optional end param
sub = buffer.slice(5);

assert.ok(sub instanceof ArrayBuffer);
assert.equal(sub.byteLength, 3);
for (i = 0; i < 3; i++) {
  assert.equal(sub[i], i+5);
}

// test clamping
sub = buffer.slice(-3, -1);

assert.ok(sub instanceof ArrayBuffer);
assert.equal(sub.byteLength, 2);
for (i = 0; i < 2; i++) {
  assert.equal(sub[i], i+5);
}

// test invalid clamping range
sub = buffer.slice(-1, -3);

assert.ok(sub instanceof ArrayBuffer);
assert.equal(sub.byteLength, 0);

// test wrong number of arguments
var gotError = false;

try {
  sub = buffer.slice();
} catch (e) {
  gotError = true;
}

assert.ok(gotError);