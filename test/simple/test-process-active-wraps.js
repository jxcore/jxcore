// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var spawn = require('child_process').spawn;
var net = require('net');

function expect(activeHandles, activeRequests) {
  assert.equal(process._getActiveHandles().length, activeHandles);
  assert.equal(process._getActiveRequests().length, activeRequests);
}

var handles = [];

(function() {
  expect(0, 0);
  var server = net.createServer().listen(common.PORT);
  expect(1, 0);
  server.close();
  expect(1, 0); // server handle doesn't shut down until next tick
  handles.push(server);
})();

(function() {
  expect(1, 0);
  var conn = net.createConnection(common.PORT);
  conn.on('error', function() { /* ignore */ });
  expect(2, 1);
  conn.destroy();
  expect(2, 1); // client handle doesn't shut down until next tick
  handles.push(conn);
})();

(function() {
  var n = 0;
  handles.forEach(function(handle) {
    handle.once('close', onclose);
  });
  function onclose() {
    if (++n === handles.length) setImmediate(expect, 0, 0);
  }
})();