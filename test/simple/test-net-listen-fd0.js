// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var net = require('net');

var gotError = false;

process.on('exit', function() {
  assert.equal(gotError, true);
});

// this should fail with an async EINVAL error, not throw an exception
net.createServer(assert.fail).listen({fd:0}).on('error', function(e) {
  assert.equal(e.code, 'EINVAL');
  gotError = true;
});