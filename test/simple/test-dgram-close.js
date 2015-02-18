// Copyright & License details are available under JXCORE_LICENSE file


// Ensure that if a dgram socket is closed before the DNS lookup completes, it
// won't crash.

var assert = require('assert'),
    common = require('../common'),
    dgram = require('dgram');

var buf = new Buffer(1024);
buf.fill(42);

var socket = dgram.createSocket('udp4');
var handle = socket._handle;
socket.send(buf, 0, buf.length, common.PORT, 'localhost');
socket.close();
socket = null;

// Verify that accessing handle after closure doesn't throw
setImmediate(function() {
  setImmediate(function() {
    console.log('Handle fd is: ', handle.fd);
  });
});