// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating an http server and a client tries to connect to it.
 */


var jx = require('jxtools');
var assert = jx.assert;
var http = require("http");

var finished = false;
var port = 8126;
var clientReceived = false;

var done = function() {
  assert.ok(finished, "Test unit did not finish.");
  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(clientReceived, sid + "Client did not receive message from the server on thread");
  if (process.subThread)
    process.release();
};

var finish = function (req) {
  if (req) {
    req.abort();
  }
  srv.unref();
  finished = true;
  done();
};

// ########   server

var srv = http.createServer(function (req, res) {
  // sending back to client
  res.end("ok");
});

srv.on('error', function (e) {
  console.error("Server error: \n" + e);
  done();
});

srv.on("listening", function () {
  client();
});
srv.listen(port, "localhost");


// ########   client

var client = function () {
  var options = {
    hostname: 'localhost',
    port: port,
    path: '/',
    method: 'POST'
  };

  var req = http.get(options, function () {
    clientReceived = true;
    finish(req);
  });

  req.on("error", function (err) {
    console.error("Client error: \n" + err);
    finish(req);
  });
};

// if done() was not called already...
setTimeout(done, 10000).unref();