// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating an http server and a client tries to connect to it.
 */

var jx = require('jxtools');
var assert = jx.assert;
var http = require("http");

//if (!jx_commmon.allowOnlyMT(false, false))
//    return;

var finished = false;
var port = 8126;
var clientReceived = false;


// ########   server

var srv = http.createServer(function (req, res) {
  // sending back to client
  res.end("ok");
});

srv.on('error', function (e) {
  jx.throwMT("Server error: \n" + e);
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
    req.abort();
    srv.close();
    finished = true;
  });

  req.on("error", function (err) {
    finish = true;
    assert.ifError(err, "Client error: \n" + err);
  });
};


process.on("exit", function (code) {
  assert.ok(finished, "Test unit did not finish.");

  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(clientReceived, sid + "Client did not receive message from the server.");
});