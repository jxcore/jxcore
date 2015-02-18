// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

var net = require('net');

var expected_bad_connections = 1;
var actual_bad_connections = 0;

var host = '********';
host += host;
host += host;
host += host;
host += host;
host += host;

function do_not_call() {
  throw new Error('This function should not have been called.');
}

var socket = net.connect(42, host, do_not_call);
socket.on('error', function(err) {
  assert.equal(err.code, 'ENOTFOUND');
  actual_bad_connections++;
});

process.on('exit', function() {
  assert.equal(actual_bad_connections, expected_bad_connections);
});