// Copyright & License details are available under JXCORE_LICENSE file

var port = 8124;
var started = false;

var net = require('net');
var server = net.createServer();
var jx = require('jxtools');
var assert = jx.assert;

if (process.threadId != -1)
  process.keepAlive();

process.on('exit', function (code) {
  //console.log("Exiting " );
  assert.ok(started, "Client did not connect to the server.")
});


server.listen(port, function () {
  //console.log("started" + process.threadId);
  started = true;
  server.close();

  if (process.threadId !== -1) {
    process.release();
    process.release();
  }
});

server.on("error", function (err) {
  assert.ifError(err, err);
});



