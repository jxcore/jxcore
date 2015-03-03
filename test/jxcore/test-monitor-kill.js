// Copyright & License details are available under JXCORE_LICENSE file

if (process.IsEmbedded)
  return;

var http = require("http"),
  fs = require('fs'),
  path = require("path"),
  childprocess = require("child_process"),
  assert = require('assert');

var port = 17777;
var finished = false;
var subscribed = false;
var killed = false;


// monitored app will just create an http server
var baseFileName = "test-monitor-run-app-tmp.js";
var appFileName = __dirname + path.sep + baseFileName;

var str = 'require("http").createServer().listen(8587, "localhost");\n';
str += 'setTimeout(process.exit, 10000);';  // let it end after 10 secs
fs.writeFileSync(appFileName, str);

var cmd = '"' + process.execPath + '" monitor ';

process.on('exit', function (code) {
  jxcore.utils.cmdSync(cmd + 'stop');
  assert.ok(finished, "Test unit did not finish.");
  assert.ok(subscribed, "Application did not subscribe to a monitor with `jx monitor run` command.");
  assert.ok(killed, "Application was not killed with `jx monitor kill` command.");

  var _cmd = process.platform == 'win32' ? 'del /q ' : 'rm -f ';
  jxcore.utils.cmdSync(_cmd + "*monitor*.log");
  fs.unlinkSync(appFileName);
});

// calls monitor and gets json: http://localhost:17777/json
var getJSON = function (cb) {
  var options = {
    host: 'localhost',
    port: port,
    path: '/json'
  };

  http.get(options, function (res) {
    res.setEncoding('utf8');
    res.on('data', function (chunk) {
      cb(false, chunk.toString());
    });
  })
    .on("error", function (err) {
      cb(true, err);
    });
};


// we use setTimeout() here, because we cannot be sure on Windows,
// that cmdSync will not exit sooner that jx monitor cmd will completejx monit
// so after it completes we wait for another 1 second

// ########################## jx monitor start
var ret = jxcore.utils.cmdSync(cmd + "start");
assert.ok(ret.exitCode === 0, "Monitor did not start after `start` command. \n", JSON.stringify(ret));

// ########################## jx monitor run test-monitor-run-app.js

var child = childprocess.exec(cmd + "run " + appFileName);

setTimeout(function () {
  getJSON(function (err, txt) {

    // should be no error and json should be returned with subscribed application data
    // including "pid" number
    if (!err && txt && txt.length && txt.indexOf(baseFileName) > -1) {
      subscribed = true;
    }

    // app subscribed to the monitor
    jxcore.utils.cmdSync(cmd + "kill " + appFileName);

    getJSON(function (err, txt) {
      // should be no error and json should be returned with only []
      if (!err && txt === "[]") {
        killed = true;
      }
      finished = true;
    });
  });
}, 3000);