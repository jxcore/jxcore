// Copyright & License details are available under JXCORE_LICENSE file


// This test starts two clustered HTTP servers on the same port. It expects the
// first cluster to succeed and the second cluster to fail with EADDRINUSE.

var common = require('../common');
var assert = require('assert');
var cluster = require('cluster');
var fork = require('child_process').fork;
var http = require('http');

var id = process.argv[2];

if (!id) {
  var a = fork(__filename, ['one']);
  var b = fork(__filename, ['two']);

  a.on('message', function(m) {
    assert.equal(m, 'READY');
    b.send('START');
  });

  var ok = false;

  b.on('message', function(m) {
    assert.equal(m, 'EADDRINUSE');
    a.kill();
    b.kill();
    ok = true;
  });

  process.on('exit', function() {
    a.kill();
    b.kill();
    assert(ok);
  });
}
else if (id === 'one') {
  if (cluster.isMaster) cluster.fork();
  http.createServer(assert.fail).listen(common.PORT, function() {
    process.send('READY');
  });
}
else if (id === 'two') {
  if (cluster.isMaster) cluster.fork();
  process.on('message', function(m) {
    assert.equal(m, 'START');
    var server = http.createServer(assert.fail);
    server.listen(common.PORT, assert.fail);
    server.on('error', function(e) {
      assert.equal(e.code, 'EADDRINUSE');
      process.send(e.code);
    });
  });
}
else {
  assert(0); // bad command line argument
}