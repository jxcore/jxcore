// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating an http server and a client tries to connect to it.
 */


var jx = require('jxtools');
var assert = jx.assert;

//if (!jx_commmon.allowOnlyMT(false, true, null, false))
//    return;

var http = require("http");


var finished = false;
var port = 8126;
var clientReceived = false;


var finish = function (req) {
  if (req) {
    req.abort();
  }
  srv.unref();
  if (process.threadId !== -1)
    process.release();
  finished = true;
};

// ########   server

var srv = http.createServer(function (req, res) {
  // sending back to client
  res.end("ok");
//    console.log("server received on thread " + process.threadId);
});

srv.on('error', function (e) {
  console.error("Server error: \n" + e);
  process.exit();
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
//        console.log("client received on thread " + process.threadId);
    finish(req);
  });

  req.on("error", function (err) {
    console.error("Client error: \n" + err);
    finish(req);
  });
};


process.on("exit", function (code) {
  assert.ok(finished, "Test unit did not finish.");

  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(clientReceived, sid + "Client did not receive message from the server on thread");
});