// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');

var str = "Hello World";
var bf = new Buffer(str);

assert.strictEqual(bf.indexOf("3"), -1);
assert.strictEqual(bf.indexOf("World"), 6);

var bfi = new Buffer("o W");
assert.strictEqual(bf.indexOf(bfi), 4);

assert.strictEqual(bf.indexOf(new Buffer(2)), -1);

var bf20 = new Buffer(20);
bf20.fill(' ');
assert.strictEqual(bf.indexOf(bf20), -1);

assert.strictEqual(bf.indexOf(111), 4);

var bf2 = new Buffer("Hello World");
assert.strictEqual(Buffer.compare(bf, bf2), 0);
assert.strictEqual(bf.compare(bf, bf2), 0);

var bf3 = new Buffer("Hello Worl ");
assert.strictEqual(Buffer.compare(bf, bf3), 1);
assert.strictEqual(bf.compare(bf3), 1);

var bf4 = new Buffer("Hello Worle");
assert.strictEqual(Buffer.compare(bf, bf4), -1);
assert.strictEqual(bf.compare(bf4), -1);

var bf5 = new Buffer("Hello");
assert.strictEqual(Buffer.compare(bf, bf5), 1);
assert.strictEqual(bf.compare(bf5), 1);