// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

switch (process.argv[2]) {
  case undefined:
    return parent();
  case 'child':
    return child();
  default:
    throw new Error('wtf');
}

function parent() {
  var http = require('http');
  if(http.setMaxHeaderLength)
    http.setMaxHeaderLength(0);
  var bigResponse = new Buffer(10240)
  bigResponse.fill('x');
  var gotTimeout = false;
  var childClosed = false;
  var requests = 0;
  var connections = 0;

  var server = http.createServer(function(req, res) {
    requests++;
    res.setHeader('content-length', bigResponse.length);
    res.end(bigResponse);
  });

  server.on('connection', function(conn) {
    connections++;
  });

  // kill the connection after a bit, verifying that the
  // flood of requests was eventually halted.
  server.setTimeout(500, function(conn) {
    gotTimeout = true;
    conn.destroy();
  });

  server.listen(common.PORT, function() {
    var spawn = require('child_process').spawn;
    var args = [__filename, 'child'];
    var child = spawn(process.execPath, args, { stdio: 'inherit' });
    child.on('close', function(code) {
      assert(!code);
      childClosed = true;
      server.close();
    });
  });

  process.on('exit', function() {
    assert(gotTimeout);
    assert(childClosed);
    assert.equal(connections, 1);
    // 1213 works out to be the number of requests we end up processing
    // before the outgoing connection backs up and requires a drain.
    // however, to avoid being unnecessarily tied to a specific magic number,
    // and making the test brittle, just assert that it's "a lot", which we
    // can safely assume is more than 200.
	if(requests<200){
	  console.log("total requests were", requests, " it should be 200");
	}
    assert(requests >= 200);
    console.log('ok');
  });
}

function child() {
  var net = require('net');

  var gotEpipe = false;
  var conn = net.connect({ port: common.PORT });

  var req = 'GET / HTTP/1.1\r\nHost: localhost:' +
            common.PORT + '\r\nAccept: */*\r\n\r\n';

  req = new Array(10241).join(req);

  conn.on('connect', function() {
    write();
  });

  conn.on('drain', write);

  conn.on('error', function(er) {
    gotEpipe = true;
  });

  process.on('exit', function() {
    assert(gotEpipe);
    console.log('ok - child');
  });

  function write() {
    while (false !== conn.write(req, 'ascii'));
  }
}