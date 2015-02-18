// Copyright & License details are available under JXCORE_LICENSE file

var assert = require("assert");
var express = require('express');
var fs = require("fs");
var pathModule = require("path");
var http = require("http");

var port = 8000;
var finished = false;

process.on("exit", function (code) {
  assert.ok(finished, "Test did not finish!");
});

// -------------   express server

var app = express();
app.use(function (req, res, next) {

  var path = pathModule.normalize(__dirname + req.url);

  if (fs.existsSync(path)) {
    // sendfile is depreciated
    // new name is sendFile
    var methodName = res.sendFile ? "sendFile" : "sendfile";
    try {
      res[methodName](path);
    } catch (ex) {
      throw "Exception during " + methodName + "('" + path + "'):\n" + ex;
      res.send("");
    }
  } else {
    res.send("This is not an asset file.");
  }
});

app.listen(8000, function () {
  console.log("Express app listening.");
  client();
});


// -------------   express client

var client = function () {
  var options = {
    hostname: 'localhost',
    port: port,
    path: "/assets/bar.txt",
    method: 'GET'
  };

  var req = http.request(options, function (res) {

    var body = ""
    res.on('data', function (chunk) {
      console.log("Client's response on('data'). Received chunk: " + chunk);
      body += chunk;
    });

    res.on('end', function () {
      console.log("Client's response on('end'). Received body: " + body);
      var txt = fs.readFileSync(pathModule.normalize(__dirname + options.path)).toString();
      assert.strictEqual(body, txt, "Express app returned `" + body + "` instead of `" + txt + "`");

      finished = true;
      process.exit();
    });
  });

  req.on("error", function (err) {
    throw "Client error: \n" + err;
    finished = true;
    process.exit();
  });

  console.log("Client is requesting " + options.path);
  req.end(0);
};