// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var net = require('net');
var assert = require('assert');

var sock = new net.Socket();

var server = net.createServer().listen(common.PORT, function() {
  assert(!sock.readable);
  assert(!sock.writable);
  assert.equal(sock.readyState, 'closed');

  sock.connect(common.PORT, function() {
    assert.equal(sock.readable, true);
    assert.equal(sock.writable, true);
    assert.equal(sock.readyState, 'open');

    sock.end();
    assert(!sock.writable);
    assert.equal(sock.readyState, 'readOnly');

    server.close();
    sock.on('close', function() {
      assert(!sock.readable);
      assert(!sock.writable);
      assert.equal(sock.readyState, 'closed');
    });
  });

  assert.equal(sock.readyState, 'opening');
});