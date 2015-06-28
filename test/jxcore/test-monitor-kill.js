// Copyright & License details are available under JXCORE_LICENSE file

if (process.isPackaged)
  return;

var http = require("http"),
  fs = require('fs'),
  path = require("path"),
  childprocess = require("child_process"),
  assert = require('assert'),
  jxtools = require("jxtools");

jxtools.listenForSignals();

var finished = false;
var subscribed = false;
var killed = false;

// neutralize old tmp file
var oldLog = path.join(__dirname, "test-monitor-run-app-tmp.js");
if (fs.existsSync(oldLog))
  fs.writeFileSync(oldLog, "");

// monitored app will just create an http server
var baseFileName = "__test-monitor-run-app-tmp.js";
var logFileName = "__test-monitor-run-app-tmp-monitor.log";
var appFileName = __dirname + path.sep + baseFileName;
jxtools.addFilesForDeletion(appFileName);

var str = 'require("http").createServer().listen(8587, "localhost");\n';
str += 'setTimeout(process.exit, 100000);';  // let it end after 10 secs
fs.writeFileSync(appFileName, str);

var cmd = '"' + process.execPath + '" monitor ';

// kill monitor if it stays as dummy process
jxcore.utils.cmdSync(cmd + "stop");

process.on('exit', function (code) {
  jxcore.utils.cmdSync(cmd + 'stop');
  jxtools.rmfilesSync("*monitor*.log");

  if (!jxtools.gotSignal) {
    assert.ok(finished, "Test unit did not finish.");
    assert.ok(subscribed, "Application did not subscribe to a monitor with `jx monitor run` command.");
    assert.ok(killed, "Application was not killed with `jx monitor kill` command.");
  }
});

// ########################## jx monitor start
var ret = jxcore.utils.cmdSync(cmd + "start");
assert.ok(ret.exitCode <= 0, "Monitor did not start after `start` command. \n", JSON.stringify(ret));

// ########################## jx monitor run test-monitor-run-app.js

var batchName = jxtools.saveBatchFile(process.execPath + " monitor run " + appFileName);

// this should be launched in background. & at the end of cmd does not work proper on windows
var out = fs.openSync(logFileName, 'a');
var err = fs.openSync(logFileName, 'a');
var child = childprocess.spawn(batchName, { detached: true, stdio: [ 'ignore', out, err ] });
child.unref();

var start = Date.now();

var check = function() {
  jxtools.getMonitorLog(function (err, txt) {

    if (!err && txt) {
      var arr = txt.split("\n");

      // checking monitor's log
      for(var a = 0, len = arr.length; a < len; a++) {
        var line = arr[a];
        var hasFile = line.indexOf(path.basename(appFileName)) > -1;

        if (hasFile) {
          if (!subscribed && line.indexOf("Received data from process.") > -1) {
            subscribed = true;
            jxcore.utils.cmdSync(cmd + "kill " + appFileName);
            break;
          }

          if (subscribed && line.indexOf("CLOSED") === 0)
            killed = true;
        }
      }
    }

    if (subscribed && killed) {
      finished = true;
      return;
    }

    if (Date.now() - start < 20000)
      setTimeout(check, 1000);
    else
      finished = true;
  });
};

process.nextTick(check);


