// Copyright & License details are available under JXCORE_LICENSE file

var port = 8124;
var started = false;
var closed = false;

var net = require('net');
var server = net.createServer();
var jx = require('jxtools');
var assert = jx.assert;


if (jx.onlyForMT())
  return;

// call keepAlive() only when `jx mt`
if (process.threadId != -1 && !jx.mtkeeped) {
  process.keepAlive();
}

process.on('exit', function (code) {
  assert.ok(started, 'Server did not start on thread ' + process.threadId);
  assert.ok(closed, 'Server did not close on thread ' + process.threadId);
});

server.listen(port, function () {
  started = true;
  server.on('close', function () {
    closed = true;
    process.release();
  });

  jxcore.store.shared.set('started' + process.threadId, 'ok');
});

server.on("error", function (err) {
  assert.ifError(err, err);
});


var interval = setInterval(function () {

  var total = jxcore.tasks.getThreadCount();
  var cnt = 0;
  for (var a = 0; a < total; a++)
    if (jxcore.store.shared.exists('started' + process.threadId))
      cnt++;

  // close the server only after all threads started
  if (cnt === total) {
    clearInterval(interval);
    server.close();
  }
}, 100);


