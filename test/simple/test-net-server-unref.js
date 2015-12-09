// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

var net = require('net');
var closed = false;

var s = net.createServer();
s.listen();
s.unref();

setTimeout(function() {
  closed = true;
  s.close();
}, 1000).unref();

process.on('exit', function() {
  assert.strictEqual(closed, false, 'Unrefd socket should not hold loop open');
});