// Copyright & License details are available under JXCORE_LICENSE file



var common = require('../common');
var assert = require('assert');
var cluster = require('cluster');
var path = require('path');
var fs = require('fs');
var net = require('net');

var socketPath = path.join(common.fixturesDir, 'socket-path');

if (cluster.isMaster) {
  var worker = cluster.fork();
  var gotError = 0;
  worker.on('message', function(err) {
    gotError++;
    console.log(err);
    if (process.platform === 'win32')
      assert.strictEqual('EACCES', err.code);
    else
      assert.strictEqual('EADDRINUSE', err.code);
    worker.disconnect();
  });
  process.on('exit', function() {
    console.log('master exited');
    try {
      fs.unlinkSync(socketPath);
    } catch (e) {
    }
    assert.equal(gotError, 1);
  });
} else {
  fs.writeFileSync(socketPath, 'some contents');

  var server = net.createServer().listen(socketPath, function() {
    console.log('here');
  });

  server.on('error', function(err) {
    process.send(err);
  });
}