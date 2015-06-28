// Copyright & License details are available under JXCORE_LICENSE file

// This module is able to run tests in similar way like with python, but without a python.
// It's useful on platforms, where python is not installed/available, e.g. android.
// jxcore$ jx test/test.js jxcore

var fs = require('fs'), path = require("path"), cp = require("child_process"), util = require("util");

var folders = []; // 'simple', 'pummel', 'internet' 'jxcore'

for (var i = 2, len = process.argv.length; i < len; i++) {
  var p = path.join(__dirname, process.argv[i]);
  if (!fs.existsSync(p))
    jxcore.utils.console.error("Unknown test folder:", process.argv[i]);
  else
    folders.push(process.argv[i]);
}

if (!folders.length) {
  console.log("Please provide test folder name(s), e.g.: `jxcore`");
  process.exit();
}

// we need one empty line at start
console.log("\n");

var allFiles = [], cwd = process.cwd() + path.sep, dir = cwd + "test"
    + path.sep, cursorUp = "\033[1A", clearLine = "\033[K", stats = {
  total : 0,
  done : 0,
  percent : 0,
  passed : 0,
  failed : 0,
  time_started : null,
  minutes_ellapsed : null,
  seconds_ellapsed : null
};

var add = function(jsFile, json_arg) {

  stats.total++;

  var obj = {
    execArgv : json_arg && json_arg.execArgv ? json_arg.execArgv + " " : "",
    argv : json_arg && json_arg.argv ? " " + json_arg.argv : "",
    fname : path.normalize(jsFile)
  };

  obj.cmd = '"' + process.execPath + '" ' + obj.execArgv + obj.fname + obj.argv;
  allFiles.push(obj);
};

for (var a = 0, len = folders.length; a < len; a++) {

  var fullDir = dir + folders[a] + path.sep;
  var files = fs.readdirSync(fullDir);

  for (var b = 0, len2 = files.length; b < len2; b++) {
    if (files[b].slice(0, 5) !== 'test-' || path.extname(files[b]) !== ".js")
      continue;

    if (fs.existsSync(fullDir + files[b] + ".json")) {
      var json = fs.readFileSync(fullDir + files[b] + ".json");
      json = JSON.parse(json.toString());

      for ( var o in json.args)
        add(fullDir + files[b], json.args[o]);

      continue;
    } else {
      add(fullDir + files[b]);
    }
  }
}

stats.time_started = Date.now();

var pad = function(str, cnt, char) {
  str = str + "";
  while (str.length < cnt) {
    str = char + str;
  }
  return str;
};

// writes this in the last line: [00:03|% 3|+ 21|- 3]
var writeStats = function(obj) {

  var fname = obj.fname || obj;
  var time = "%s:%s";
  var percent = jxcore.utils.console.setColor("%% %s", "blue");
  var success = jxcore.utils.console.setColor("+ %s", "green");
  var failures = jxcore.utils.console.setColor("- %s", "red");
  var s = util.format("[" + [ time, percent, success, failures ].join("|")
      + "] " + fname.replace(cwd, ""), pad(stats.minutes_ellapsed, 2, '0'),
      pad(stats.seconds_ellapsed, 2, '0'), pad(stats.percent, 3, ' '), pad(
          stats.passed, 3, ' '), pad(stats.failed, 3, ' '));

  console.log(cursorUp + clearLine + s);
};

var addStat = function(ret) {

  if (ret.exitCode) {
    stats.failed++;
    jxcore.utils.console.log(cursorUp + clearLine + "=== "
        + ret.fname.replace(cwd, "") + " ===", "cyan");
    jxcore.utils.console.log("Failure no " + stats.failed + ", exit code: "
        + ret.exitCode, "magenta");
    jxcore.utils.console.log(ret.out.toString().trim());
    jxcore.utils.console.log("Command:", ret.cmd, "yellow");
    jxcore.utils.console.log("\n");
  } else {
    stats.passed++;
  }

  var seconds_delta = (Date.now() - stats.time_started) / 1000;
  stats.minutes_ellapsed = Math.floor(seconds_delta / 60);
  stats.seconds_ellapsed = Math.round(seconds_delta - stats.minutes_ellapsed
      * 60);
  stats.done++;
  stats.percent = Math.round(stats.done * 100 / stats.total);

};

var runNextTest = function() {
  if (!allFiles.length) {
    writeStats("Finished.");
    return;
  }
  var rec = allFiles.shift(), ret = {
    fname : rec.fname,
    exitCode : 0,
    timeouted : false,
    cmd : rec.cmd
  };

  var debug = false;

  writeStats(ret);
  var child = cp.exec(rec.cmd, {
    timeout : 20000
  }, function(error, stdout, stderr) {
    ret.out = stdout.toString() + stderr.toString();
    ret.err = stderr.toString();
    ret.error = error;
    if (debug) {
      console.log("on finish, ret", ret, "\n");
    }
    addStat(ret);
    setTimeout(runNextTest, 10);
  });

  child.on('exit', function(code) {
    if (debug) {
      jxcore.utils.console.log("on exit, code " + code, "yellow");
    }
    ret.exitCode = code;
    ret.timeouted = code == 143; // SIGTERM is called when timeout occurs
    child.kill();
    child = null;
  });
};

runNextTest();
