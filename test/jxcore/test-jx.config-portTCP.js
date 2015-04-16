// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating an http server and a client tries to connect to it.
 */

var jx = require('jxtools');
var assert = jx.assert;
var http = require("http");

var finished = false;
var port = 8126;
var portFromConfigFile = 8976; // <-- value from jxcore.config
var clientReceived = false;

var done = function() {
  assert.ok(finished, "Test unit did not finish.");
  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(clientReceived, sid + "Client did not receive message from the server on port " + portFromConfigFile);
  if (process.subThread)
    process.release();
};

var finish = function (req) {
  srv.unref();
  finished = true;
  if (req) {
    req.abort();
    done();
  }
};


// ########   server

var srv = http.createServer(function (req, res) {
  // sending back to client
  res.end("ok");
  assert.strictEqual(portFromConfigFile, req.socket.localPort, "Port from config file (" + portFromConfigFile + ") is different than req.socket.localPort (" + req.socket.localPort + ")");
  finish();
});

srv.on('error', function (e) {
  assert(0, "Server error: \n" + e);
  finish();
});

srv.on("listening", function () {
  client();
});
srv.listen(port, "localhost");


// ########   client

var client = function (port) {
  var options = {
    hostname: 'localhost',
    port: portFromConfigFile,
    path: '/',
    method: 'POST'
  };

  var req = http.get(options, function () {
    clientReceived = true;
    finish(req);
  }).on("error", function (err) {
    assert(0, "Client error: cannot connect on port defined in .jxcore.config (" + portFromConfigFile + ")\n" + err);
    finish(req);
  });
};

// if done() was not called already...
setTimeout(done, 10000).unref();

