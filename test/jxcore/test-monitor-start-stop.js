// Copyright & License details are available under JXCORE_LICENSE file


if (process.IsPackaged)
  return;

var http = require("http"),
  fs = require('fs'),
  path = require("path"),
  assert = require('assert');

var port = 17777;
var finished = false;

var cmd = '"' + process.execPath + '" monitor ';

// kill monitor if stays as dummy process
jxcore.utils.cmdSync(cmd + "stop");

process.on('exit', function (code) {
  jxcore.utils.cmdSync(cmd + 'stop');
  var _cmd = process.platform == 'win32' ? 'del /q ' : 'rm -f ';
  jxcore.utils.cmdSync(_cmd + "*monitor*.log");
  assert.ok(finished, "Test unit did not finish.");
});

// this is for testing if http port is taken by monitor process or not
var createSrv = function (cb) {
  var srv = http.createServer(function (req, res) {
  });

  srv.on('error', function (e) {
    cb(e);
  });

  srv.on("listening", function () {
    cb();
    srv.close();
  });
  srv.listen(port, "127.0.0.1");
};

// we use setTimeout() here, because we cannot be sure on Windows,
// that cmdSync will not exit sooner that jx monitor cmd will complete
// so after it completes we wait for another 1 second

// ########################## jx monitor start
jxcore.utils.cmdSync(cmd + "start");
setTimeout(function () {
  createSrv(function (err) {
    // should be error
    if (err) {

      // ########################## jx monitor restart
      jxcore.utils.cmdSync(cmd + "restart");

      setTimeout(function () {
        createSrv(function (err) {
          // should be error
          if (err) {
            jxcore.utils.cmdSync(cmd + "stop");

            // ########################## jx monitor stop
            setTimeout(function () {
              createSrv(function (err) {
                // should not be error
                assert.ifError(err, "Monitor was not stopped after `stop` command.");
                finished = true;
              })
            }, 1);


          } else {
            throw "Monitor did not start after `restart` command.";
          }
        });
      }, 1);


    } else {
      throw "Monitor did not start after `start` command.";
    }
  });
}, 1);