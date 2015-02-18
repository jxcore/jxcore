// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');
var util = require('util');

assert.equal(util.format(), '');
assert.equal(util.format(''), '');
assert.equal(util.format([]), '[]');
assert.equal(util.format({}), '{}');
assert.equal(util.format(null), 'null');
assert.equal(util.format(true), 'true');
assert.equal(util.format(false), 'false');
assert.equal(util.format('test'), 'test');

// CHECKME this is for console.log() compatibility - but is it *right*?
assert.equal(util.format('foo', 'bar', 'baz'), 'foo bar baz');

assert.equal(util.format('%d', 42.0), '42');
assert.equal(util.format('%d', 42), '42');
assert.equal(util.format('%s', 42), '42');
assert.equal(util.format('%j', 42), '42');

assert.equal(util.format('%d', '42.0'), '42');
assert.equal(util.format('%d', '42'), '42');
assert.equal(util.format('%s', '42'), '42');
assert.equal(util.format('%j', '42'), '"42"');

assert.equal(util.format('%%s%s', 'foo'), '%sfoo');

assert.equal(util.format('%s'), '%s');
assert.equal(util.format('%s', undefined), 'undefined');
assert.equal(util.format('%s', 'foo'), 'foo');
assert.equal(util.format('%s:%s'), '%s:%s');
assert.equal(util.format('%s:%s', undefined), 'undefined:%s');
assert.equal(util.format('%s:%s', 'foo'), 'foo:%s');
assert.equal(util.format('%s:%s', 'foo', 'bar'), 'foo:bar');
assert.equal(util.format('%s:%s', 'foo', 'bar', 'baz'), 'foo:bar baz');
assert.equal(util.format('%%%s%%', 'hi'), '%hi%');
assert.equal(util.format('%%%s%%%%', 'hi'), '%hi%%');