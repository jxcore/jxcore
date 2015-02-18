// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

assert.ok(process.stdout.writable);
assert.ok(process.stderr.writable);
// Support legacy API
assert.equal('number', typeof process.stdout.fd);
assert.equal('number', typeof process.stderr.fd);


var stdout_write = global.process.stdout.write;
var strings = [];
global.process.stdout.write = function(string) {
  strings.push(string);
};

console.log('foo');
console.log('foo', 'bar');
console.log('%s %s', 'foo', 'bar', 'hop');
console.log({slashes: '\\\\'});

console._stderr = process.stdout;
console.trace('This is a %j %d', { formatted: 'trace' }, 10, 'foo');

global.process.stdout.write = stdout_write;

assert.equal('foo\n', strings.shift());
assert.equal('foo bar\n', strings.shift());
assert.equal('foo bar hop\n', strings.shift());
assert.equal("{ slashes: '\\\\\\\\' }\n", strings.shift());
assert.equal('Trace: This is a {"formatted":"trace"} 10 foo',
             strings.shift().split('\n').shift());

assert.throws(function () {
  console.timeEnd('no such label');
});

assert.doesNotThrow(function () {
  console.time('label');
  console.timeEnd('label');
});