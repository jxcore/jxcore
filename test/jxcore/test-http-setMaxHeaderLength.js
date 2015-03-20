// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing how http.setMaxHeaderLength works.
 HTTP client should throw exception when trying to write header bigger than defined limit.

 this unit should be called with two parameters
 > jx test-http-setMaxHeaderLength maxLenght length

 for example we can set maxLength on 5000 and try to send a header with length 4000:
 > jx test-http-setMaxHeaderLength 5000 4000

 if maxLength will be negative, it means that we don't want to call http.setMaxHeaderLength() at all
 */


var jx = require('jxtools');
var assert = jx.assert;
var http = require("http");

// reading params from argv
var maxHeaderLength = parseInt(process.argv[process.argv.length - 2]);
var headerlength = parseInt(process.argv[process.argv.length - 1]);

// when 0 or positive
if (maxHeaderLength >= 0) {
  http.setMaxHeaderLength(maxHeaderLength);
} else {
  http.setMaxHeaderLength(32768);
  // default value
  maxHeaderLength = 32768;
}

var buf = new Array(headerlength + 1).join("a");

// maxHeaderLength === 0 disables maxHeaderLength check
var shouldBeNoError = (maxHeaderLength === 0) || maxHeaderLength > headerlength;
var port = 8765 + process.threadId;
var error = false;
var finished = false;

// ########   server

var srv = http.createServer(function (req, res) {
  var receivedLength = Buffer.byteLength(req.headers["aaa"]);
  res.writeHead(200, {'receivedlength': receivedLength});
  res.end("ok");
});

srv.on('error', function (e) {
  jx.throwMT("Server error: \n" + e);
  finish();
});

srv.on("listening", function () {
  client();
});
srv.listen(port, "localhost");


var finish = function (req) {
  if (req) {
    req.abort();
  }
  setTimeout(function () {
    srv.unref();
  }, 700);
  if (process.threadId !== -1)
    process.release();
  finished = true;
};

// ########   client

var client = function () {
  var options = {
    hostname: 'localhost',
    port: port,
    path: '/',
    method: 'POST',
    headers: {"aaa": buf.toString()}
  };

  var req = http.request(options, function (res) {
    // server has sent size of received header
    assert.equal(res.headers.receivedlength, headerlength, "The length of header received by a server is smaller that expected. Should be " + headerlength + " but is " + res.headers.receivedlength + "\n\n");
    finish(req);
  });

  req.on("error", function (err) {
    error = true;
    if (shouldBeNoError) {
      assert.ifError(err, "client error!\n" + err);
    }
    finish(req);
  });

  if (!shouldBeNoError) {
    console.log(undefined);
    jxcore.utils.console.log("Error expected:", "green");
  }

  req.end();
};


process.on("exit", function (code) {
  assert.ok(finished, "Test unit did not finish.");
  assert.ok(shouldBeNoError === !error, "Condition failed.");
});