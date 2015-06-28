// Copyright & License details are available under JXCORE_LICENSE file

var path = require('path');
var fs = require('fs');
var mon = require('_jx_monitor');
var _tw = process.binding("jxutils_wrap");
var cc = jxcore.utils.console;

var stopIt = function(restart) {
  cc.log("Stopping JXcore monitoring service.", "cyan+bold");

  mon.checkMonitorExists(function(err, msg) {

    var _restartOrExit = function(restart, err, msg) {
      if (err && msg) console.error(err, msg);
      console.log("");

      if (restart) {
        startIt();
      } else {
        process.exit(err ? 1 : 0);
      }
    };

    if (!err) {
      mon.stopMonitor(function(err2, msg2) {
        if (!err2)
          console.log("Monitoring service is closed now.");

        _restartOrExit(restart, err2, msg2);
      });
    } else {
      console.log("Monitoring service is not online.");
      _restartOrExit(restart);
    }

  });
};

var startMon = function() {

  var logFileName = "./jxcore.monitor.console.log";

  if (fs.existsSync(logFileName))
    fs.unlinkSync(logFileName);

  var spawn = require('child_process').spawn;
  var out = fs.openSync(logFileName, 'a');
  var err = fs.openSync(logFileName, 'a');
  var cmd = process.argv[0];

  var child = spawn(cmd, ['monitor', 'startx'], {
    detached: true,
    stdio: ['ignore', out, err]
  });

  child.unref();

  setTimeout(function() {

    if (fs.existsSync(logFileName)) {
      console.log(fs.readFileSync(logFileName) + "");
      console.log("");
      // exit without error
      process.exit(0);
    } else {
      mon.checkMonitorExists(function(err, msg) {
        if (!err) {
          console.log("Monitoring service is started.");
        } else {
          console.log("Monitoring service is not online.");
        }

        if (msg) console.error(msg);

        console.log("");
        process.exit(err ? 1 : 0);
      });
    }
  }, 2000);
};

var startIt = function() {
  cc.log("Starting JXcore monitoring service.", "cyan+bold");

  mon.checkMonitorExists(function(err, msg) {
    if (!err) {
      cc.log('Monitoring is already active.', "red+bold");
      process.exit(1);
    } else {
      startMon();
    }
  });
};

try {
  var cmd = process.argv[2].trim().toLowerCase();
  if (cmd == 'run') {
    if (process.argv.length < 4) {
      cc.log("missing application file", "red+bold");
      cc.log("");
      console.log("usage: jx monitor run [applicationfile]");
      console.log("");
      process.exit(1);
    } else {
      process.argv[3] = path.resolve(process.argv[3]);

      require.main.filename = process.argv[3];
      require.main.fileSource = null;

      require('_jx_config');

      mon.followMe(function(err, msg) {
        if (err) {
          cc.log("Application monitoring is failed.", 'red+bold');
          if (msg) console.error(msg);
          console.log("");
          process.exit(1);
        } else {
          var h = require(process.argv[3]);
        }
      }, function(delay) {
        setTimeout(function() {
        }, delay + 100);
      });
    }
  } else if (cmd == 'start') {
    _tw.beforeApplicationStart({});
    startIt();
  } else if (cmd == 'startx') {
    _tw.beforeApplicationStart({});
    var mon = require('_jx_monitor');
    mon.startIfNotExists();
  } else if (cmd == 'stop') {
    _tw.beforeApplicationStart({});
    stopIt();
  } else if (cmd == 'kill') {
    _tw.beforeApplicationStart({});
    process.argv[3] = path.resolve(process.argv[3]);
    mon.releasePath(process.argv[3], function(err, msg) {
      if (err) {
        cc.log("Kill operation is failed.", 'red+bold');
        if (msg) console.error(msg);
        console.log("");
        process.exit(1);
      } else {
        console.log(msg || "Process with given path is killed.");
      }
    });
  } else if (cmd == 'restart') {
    _tw.beforeApplicationStart({});
    stopIt(true);
  }
} catch (e) {
  cc.log("Error:" + e, 'red+bold');
  console.log('');
  process.exit(1);
}