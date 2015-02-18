// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit tests http server starting and listening
 */


var jx = require('jxtools');
var assert = jx.assert;
var http = require("http");


var finished = false;
var listening = true;
var port = 8126;


var finish = function () {
  srv.unref();
  if (process.threadId != -1)
    process.release();
  finished = true;
};


// ########   server

var srv = http.createServer();

srv.on('error', function (e) {
  assert.ifError(e, "Server error: \n" + e);
  finish();
});

srv.on("listening", function () {
  listening = true;
  finish();
});
srv.listen(port, "localhost");


process.on("exit", function (code) {
  assert.ok(finished, "Test unit did not finish.");

  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(listening, sid + "Server did not start to listen.");
  console.log("sss");
  jx.exitNowMT();
});