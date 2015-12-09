// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var net = require('net');
var fs = require('fs');
var path = require('path');

var serverConnected = false;
var clientConnected = false;

var options = {
  key: fs.readFileSync(path.join(common.fixturesDir, 'test_key.pem')),
  cert: fs.readFileSync(path.join(common.fixturesDir, 'test_cert.pem'))
};

var server = tls.createServer(options, function(socket) {
  serverConnected = true;
  socket.end('Hello');
}).listen(common.PORT, function() {
  var socket = net.connect({
    port: common.PORT,
    rejectUnauthorized: false
  }, function() {
    var client = tls.connect({
      rejectUnauthorized: false,
      socket: socket
    }, function() {
      clientConnected = true;
      var data = '';
      client.on('data', function(chunk) {
        data += chunk.toString();
      });
      client.on('end', function() {
        assert.equal(data, 'Hello');
        server.close();
      });
    });
  });
});

process.on('exit', function() {
  assert(serverConnected);
  assert(clientConnected);
});