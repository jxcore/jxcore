// Copyright & License details are available under JXCORE_LICENSE file

var port = 8124;
var started = false;
var closed = false;

var net = require('net');
var server = net.createServer();
var jx = require('jxtools');
var assert = jx.assert;

if (jx.onlyForSingleThread())
  return;

process.on('exit', function (code) {
  assert.ok(started, 'Server did not start.');
  assert.ok(closed, 'Server did not close.');
});

server.listen(port, function () {
  started = true;
  server.on('close', function () {
    closed = true;
  });
  server.close();
});

server.on("error", function (err) {
  assert.ifError(err, err);
});



