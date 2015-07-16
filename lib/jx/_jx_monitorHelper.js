// Copyright & License details are available under JXCORE_LICENSE file

var path = require('path');
var fs = require('fs');
var mon = require('_jx_monitor');
var _tw = process.binding("jxutils_wrap");
var cc = jxcore.utils.console;
var subscribeOkString = "Native package has been successfully subscribed to the monitor and runs as a separate process.";
var quitedTooFastString = "Application did not subscribe to the monitor, because it completes under start_delay interval.";

var stopIt = function(restart) {
  cc.log("Stopping JXcore monitoring service.", "cyan+bold");

  mon.checkMonitorExists(function(err, msg) {

    var _restartOrExit = function(restart, err, msg) {
      if (err && msg)
        console.error(err, msg);
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

  var child = spawn(cmd, [ 'monitor', 'startx' ], {
    detached : true,
    stdio : [ 'ignore', out, err ]
  });

  child.unref();

  setTimeout(function() {

    if (fs.existsSync(logFileName)) {
      console.log(fs.readFileSync(logFileName).toString().trim());
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

        if (msg)
          console.error(msg);

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

var followNativePackage = function() {
  // spawning native package with process.env.JX_MONITOR_RUN variable set
  // it will subscribe to monitor internally
  process.env.JX_MONITOR_RUN = true;

  // cutting out 4 params: jx monitor run native_file
  var argv = process.argv.slice(4);
  var fname = path.join(__dirname, "./jxcore."
      + path.basename(process.mainModule.filename) + ".log");
  if (fs.existsSync(fname))
    fs.unlinkSync(fname);

  var spawn = require('child_process').spawn, out = fs.openSync(fname, 'a'), err = fs
      .openSync(fname, 'a'), dontCheck = false;

  // skip "jx monitor run"
  var child = spawn(process.argv[3], argv, {
    detached : true,
    stdio : [ 'ignore', out, err ]
  });

  var killAndExit = function(err, msg) {
    dontCheck = true;
    if (err)
      cc.error(err);
    if (msg)
      cc.log(msg);
    try {
      if (child)
        child.kill();
    } catch (ex) {
    }
    process.exit(err ? 1 : 0);
  };

  child.on('exit', function(code) {
    var msg = null;
    var err = null;
    if (code) {
      err = "Application exited with exitCode " + code + ".";
      try {
        msg = fs.readFileSync(fname).toString();
      } catch (ex) {
      }
      ;
    } else {
      var str = readLog();
      if (str && str.indexOf(quitedTooFastString) !== -1)
        msg = quitedTooFastString;
    }
    killAndExit(err, msg);
  });

  child.on('error', function(err) {
    killAndExit("Cannot spawn a native package:\n" + err);
  });

  var readLog = function() {
    try {
      return fs.readFileSync(fname).toString();
    } catch (ex) {
    }
    return null;
  };

  var checkLog = function() {
    if (dontCheck)
      return;
    var str = readLog();
    if (str && str.indexOf(subscribeOkString) !== -1) {
      // may exit now, and leave the child running,
      // since the child has successfully subscribed
      cc.log(subscribeOkString);
      child.unref();
      process.nextTick(process.exit);
    }
    if (new Date() - start > 5000) {
      killAndExit("Timeout on application subscribing to the monitor.");
    } else {
      setTimeout(checkLog, 500);
    }
  };

  var start = new Date();
  setTimeout(checkLog, 500);
};

exports.tryToFollowMeOrExit = function(cb) {

  mon.followMe(function(err, msg) {
    if (err) {
      cc.log("Application monitoring failed.", 'red+bold');
      if (msg)
        console.error(msg);
      console.log("");
      process.exit(1);
    } else {
      if (process.env.JX_MONITOR_RUN)
        cc.log(subscribeOkString);

      if (cb)
        cb();
    }
  }, function(delay) {
    setTimeout(function() {
    }, delay + 100).unref();

    var timeStamp = new Date();
    process.on('exit', function() {
      if (new Date() - timeStamp < delay) {
        cc.log(quitedTooFastString);
      }
    });

  });
};

try {
  var cmd = process.argv[2] ? process.argv[2].trim().toLowerCase() : null;
  if (cmd == 'run') {
    if (process.argv.length < 4) {
      cc.log("missing application file", "red+bold");
      cc.log("");
      console.log("usage: jx monitor run [applicationfile]");
      console.log("");
      process.exit(1);
    } else {
      process.argv[3] = path.resolve(process.argv[3]);

      var isNativePackage = false;
      var extName = path.extname(process.argv[3]).toLowerCase();
      if (extName !== ".js" && extName !== ".jx") {
        var pkg = require("_jx_package");
        isNativePackage = pkg.isNativePackage(process.argv[3]);

        if (!isNativePackage) {
          cc
              .error("The `jx monitor run` command can work only with .js files, .jx packages and native packages.");
          cc.error("The given file is neither of them: " + process.argv[3],
              "red+bold");
          process.exit(1);
        }
      }

      require.main.filename = process.argv[3];
      require.main.fileSource = null;

      require('_jx_config');

      if (!isNativePackage) {
        // .js or .jx files
        require(process.argv[3]);
        // if app dies before start_delay, that's ok:
        // following the monitor will not take place
        exports.tryToFollowMeOrExit();
      } else {
        followNativePackage();
      }
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
        cc.log("Kill operation failed.", 'red+bold');
        if (msg)
          console.error(msg);
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