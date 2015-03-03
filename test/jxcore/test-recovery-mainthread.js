// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');
var fs = require('fs');
var path = require('path');

var logFile = process.cwd() + "/jxcore.recovery.mainthread.log";

var argv_2 = process.argv[process.argv.length - 2];
var argv_1 = process.argv[process.argv.length - 1];
var isChild = (argv_2 == "child" || argv_1 == "child");
var isChildRestarted = (argv_1 == "restarted");

var finished = false;
var restarted = false;
var start = Date.now();


if (isChild) {
  // this code is executed, when this test unit spawns itself if "child" was added to argv

  var log = function (txt) {
    txt = txt + " at " + (Date.now() - start) + " ms";
    fs.appendFileSync(logFile, txt + "\n");
    console.log(txt);
  };

  if (!isChildRestarted) {
    fs.writeFileSync(logFile, '');
    log("/********************** App first start");
  } else {
    log("/********************** App RESTARTED");
    process.exit();
  }

  log("pid" + process.pid + "dip");
  log("process.argv: " + JSON.stringify(process.argv));

  process.on('restart', function (restartCB, exit_code) {
    process.argv.push("restarted");
    log("RESTARTING. Exit code: " + exit_code);
    restartCB(33);
  });

  setTimeout(function () {
    log("Exiting to test recovery.");
    throw "some error";
  }, 5200);

  process.on('exit', function (code) {
    log('Exiting with code : ' + code);
  });

}


if (!isChild) {
  // this is main block for spawning a process, which will restart itself

  var cp = require("child_process");
  var pids = [];


  var child = cp.exec('"' + process.execPath + '" "' + __filename.replace(".js.jx", ".jx") + '" child', null, function (err, stdout, stderr) {

    var checkLog = function () {

      var log = fs.readFileSync(logFile).toString();
      // reading pids if recovered processes. they look like this: "pid1234dip"
      pids = log.match(/pid(.*)dip/g);

      if (log.indexOf("App RESTARTED") > -1) {
        // ok, process restarted
        restarted = true;
      } else {
        // if not lest wait further, but no longer than 15 secs
        if (Date.now - start > 15000) {
          if (pids.length == 1) {
            throw "The application did not RESTART at all.";
          } else {
            throw "The application did not SKIP RESTART at all.";
          }
        } else {
          setTimeout(checkLog, 2500);
        }
      }
    }

    checkLog();

    finished = true;
  });


  process.on('exit', function (code) {
    assert.ok(finished, "Test did not finish!");
    assert.ok(restarted, "Process did not restart!");

    for (var a = 0, len = pids.length; a < len; a++) {
      // just for case, if recovered processes still exist - kill them
      var pid = parseInt(pids[a].replace("pid", '').replace("dip", ''));
      try {
        process.kill(pid);
      } catch (ex) {
      }
    }

    if (fs.existsSync(logFile)) {
      fs.unlinkSync(logFile);
    }

    // internal log from inside JXcore
    var log2 = process.cwd() + path.sep + "jxcore." + path.basename(__filename) + ".log";
    if (fs.existsSync(log2)) {
      fs.unlinkSync(log2);
    }
  });

}



