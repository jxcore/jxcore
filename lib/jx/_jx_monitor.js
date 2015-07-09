// Copyright & License details are available under JXCORE_LICENSE file

var defaultMonConfig = {
  "monitor" : {
    "host" : "127.0.0.1",
    "port" : 17777,
    "check_interval" : 1000,
    // supported tags: [WEEKOFYEAR], [DAYOFMONTH], [DAYOFYEAR], [YEAR], [MONTH],
    // [MINUTE], [HOUR]
    "log_path" : "monitor_[WEEKOFYEAR]_[YEAR].log",
    "start_delay" : 2000
  }
};

var fs = require("fs"), path = require("path"), _http = require("http"), _https = require("https"), os = require("os"), util = require('util'), _url = require("url");

var http = null, https_keys = null;

var strings = {
  send_question : "jx_who_are_you",
  get_answer : "i_am_a_jx_monitor",
  send_data : "sending_data",
  get_dataOK : "thanks_for_sending_the_data",
  command_kill : "should_be_killed",
  command_refuse : "wrong_data",
  send_command : "send_command",
  send_command_hash : "commandHash",
  monitor_shutdown : "Monitor shutdown."
};

var argNames = {
  start_monitor : "start",
  stop_monitor : "stop",
  restart_monitor : "restart",
  leave_me : "leave_me",
  checkPath : "check",
  console : "console",
  monitor_old_pid : "JX_MONITOR_RESPAWN_OLD_PID",
  monitor_respawn_id : "JX_MONITOR_RESPAWN_ID"
};

var parsedArgv = jxcore.utils.argv.parse(), appConfig = null, monConfig = null, monLogPath = null, monLogDisabled = false, isWindows = process.platform === 'win32', maxAttemptCount = 10, attemptID = 0, attemptInterval = 1000, monitoredProcesses = {}, killedProcesses = {},
// true if monitor shuts down and kills the children:
massiveKiller = false, tmpLogCache = [], tmpLogCacheFlushed = false,
// true, if current process is the monitor (it created http server):
iAmTheMonitor = false, exitting = false, monitorUrl = "", childrenCheckInProgress = false, childrenStopInProgress = false;

/**
 * Writes log to the file (if can)
 * 
 * @param txt
 * @param alsoToConsole
 *          {boolean} - is used in some critical point to write msg not only to
 *          log, but also to console
 */
var monCounter = 0;
var writeToLog = function(txt, alsoToConsole) {

  // argv[argNames.console] is just for development process
  if (parsedArgv[argNames.console] || alsoToConsole)
    console.log(txt);

  txt = jxcore.utils.console.removeColors(txt);

  if (!monLogDisabled) {
    // some logging can occur before config file will be read
    // and log_path will be known so we cache some messages
    if (!tmpLogCacheFlushed) {
      tmpLogCache.push(txt);
      if (tmpLogCache.length >= 100) {
        tmpLogCache.unshift();
      }
    }
  } else {
    tmpLogCache = null;
  }

  if (monLogPath && !monLogDisabled) {
    if (monCounter++ > 1e4) {
      monCounter = 0;
      monLogPath = findLogPath();
    }
    try {
      if (!tmpLogCacheFlushed) {
        txt = tmpLogCache.join(os.EOL);
        tmpLogCacheFlushed = true;
        tmpLogCache = null;
      }
      fs.appendFileSync(monLogPath, txt + os.EOL);
    } catch (ex) {
      // cannot write to log file. Where should we display the output if not to
      // console?
    }
  }
};

var log = function(txt, alsoToConsole) {
  writeToLog(txt, alsoToConsole);
};

var logError = function(txt, ex, alsoToConsole) {
  if (txt) {
    var e = ex ? ". Exception message: " + (ex.message ? ex.message : ex) : "";
    writeToLog("Error: " + txt + e, alsoToConsole);
  }
};

var throwError = function(txt) {
  logError(txt);
  throw txt;
};

/**
 * Sends request to the monitor with url = http://host:port/path
 * 
 * @param path
 *          {string} - this is the last part of the url (http://host:port/path)
 * @param jsonData
 *          {object} - this object (stringified) will be sent by POST method
 * @param expectedAnswer
 *          {string} - The answer from monitor is compared with this value. If
 *          they are equal, sendRequest is considered to be successful
 * @param cb
 *          {function} - this callback will be invoked after sendRequest
 *          completes. Callback will receive 2 params (err true/false, txt). If
 *          error occurs, txt will have error message, otherwise, txt will have
 *          returned msg.
 */
var sendRequest = function(path, jsonData, expectedAnswer, cb) {

  var requestOptions = {
    host : monConfig.monitor.host,
    port : monConfig.monitor.port,
    path : '/' + path,
    method : 'POST',
    headers : {
      'Content-Type' : 'application/json',
      'Content-Length' : 0
    }
  };

  var str = JSON.stringify(jsonData);
  requestOptions.headers['Content-Length'] = str.length;

  var __request = function(res) {

    res.setEncoding('utf-8');

    var body = '';

    res.on('data', function(data) {
      body += data;
    });

    res
        .on(
            'end',
            function() {
              if (cb) {
                var bodyStr = body.toString();
                if (body.toString().indexOf(expectedAnswer) === -1) {
                  var refused = body.indexOf(strings.command_refuse) > -1;
                  var err = refused ? bodyStr
                      : "Problem with connecting to the monitor. Received not what expected: "
                          + bodyStr;
                  cb(true, err);
                } else {
                  cb(false, bodyStr);
                }
              }

              req.connection.destroy();
            });

    res.on('error', function(err) {
      if (cb) {
        cb(false, err);
      }
    });
  };

  var req = http.request(requestOptions, __request);

  req.on('error', function(e) {
    if (cb) {
      cb(true, e);
    }
    req.connection.destroy();
  });

  req.write(str);
  req.end();
};

/**
 * Tries to connect to monitor.
 * 
 * @param cb
 *          {function} - this callback will receive 3 params (online true/false,
 *          err, txt)
 */
var isMonitorOnline = function(cb) {
  if (!cb) {
    return;
  }

  // if port is busy, then the error occurs, but that means that our monitor
  // might be online
  sendRequest(strings.send_question, null, strings.get_answer, function(err,
      txt) {
    if (err) {
      cb(true, txt);
    } else {
      if (txt.trim().indexOf(strings.get_answer.trim()) === -1) {
        cb(true, "There is another application listening on port "
            + monConfig.monitor.port);
      } else {
        // jx monitor is online
        cb(false, null);
      }
    }
  });
};

/**
 * Reads config from given file
 * 
 * @param configFile
 *          {string} - full path
 * @return {object} - Returns config parsed as json object or nothing.
 */
var readConfig = function(configFile) {
  process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";

  var cfg = null;
  configFile = path.normalize(configFile);

  if (fs.existsSync(configFile)) {
    try {
      cfg = JSON.parse(fs.readFileSync(configFile) + "");
      if (cfg.monitor.https) {
        http = _https;
        https_keys = cfg.monitor.https;
      } else {
        http = _http;
      }
    } catch (ex) {
      logError("Cannot parse config file from " + configFile);
    }
  } else {
    logError("The config file does not exist: " + configFile);
  }
  if (!http) {
    http = _http;
  }
  return cfg;
};

var jxPath = process.execPath;

if (process.isPackaged) {
  var cmd = isWindows ? "where jx" : "which jx";
  var ret = jxcore.utils.cmdSync(cmd);
  if (!ret.exitCode) {
    var jxFile = path.normalize(ret.out.trim());
    if (fs.existsSync(jxFile)) {
      jxPath = jxFile;
      log("Found the path for jx executable: " + jxPath);
    } else {
      log("Could not find the path for jx executable: " + jxPath);
    }
  }
}

// reading monConfig. It should be read, because app should know the port, on
// which monitor is listening
monConfig = readConfig(path.dirname(jxPath) + path.sep + "jx.config") || {};

/**
 * Recursively copies fields from json_from into json_to, if they are not
 * present in json_to. This is to apply default parameters if they are not
 * present in config file.
 * 
 * @param json_from
 * @param json_to
 * @param prefix
 *          {string} - internal use. Should be empty on first call
 */
var copyConfigParams = function(json_from, json_to, prefix) {
  for ( var a in json_from) {
    if (!json_from.hasOwnProperty(a))
      continue;

    var isJSON = typeof json_from[a] == "object";
    var empty = typeof json_to[a] === "undefined";

    if (isJSON && empty) {
      json_to[a] = {};
    }
    if (!isJSON && !empty) {
      continue;
    }

    var pref = prefix ? prefix + "." + a : a;
    if (isJSON) {
      copyConfigParams(json_from[a], json_to[a], pref);
    } else {
      json_to[a] = json_from[a];
      log("Parameter '" + pref
          + "' is not present in jx.config. Using default value: '"
          + json_from[a] + "'.");
    }
  }

  monitorUrl = (https_keys ? "https://" : "http://") + monConfig.monitor.host
      + ":" + monConfig.monitor.port;
};

// applying default values
copyConfigParams(defaultMonConfig, monConfig);

/**
 * Searches for final log_path. If it's defined in jx.config, that uses this
 * value, but it may not have permissions for writing. In that case tries to
 * write the log in apps folder.
 * 
 * @return {*}
 */
var findLogPath = function() {

  if (monLogPath)
    return;

  // checking, if log file is accessible
  var now = new Date();
  var firstJanuary = new Date(now.getFullYear(), 0, 0);
  var dayOfTheYear = Math.floor((now - firstJanuary) / 86400000);
  var weekOfTheYear = Math.ceil(dayOfTheYear / 7);

  var paths = [];
  if (monConfig.monitor.log_path) {
    var logPath = monConfig.monitor.log_path;
    // var ext = path.extname(logPath);

    var dir = path.dirname(logPath).trim();
    if (dir == '.' || dir == "..") {
      // applying default dir - current directory
      logPath = process.cwd() + path.sep + logPath;
    }
    paths.push(logPath);
  }
  // next possible file name is taken from defaultMonConfig.monitor.log_path
  paths.push(process.cwd() + path.sep + defaultMonConfig.monitor.log_path);

  var firstError = "";
  for (var a = 0, len = paths.length; a < len; a++) {
    try {
      var fname = paths[a];
      fname = fname.replace(/\[WEEKOFYEAR\]/g, weekOfTheYear).replace(
          /\[YEAR\]/g, now.getFullYear()).replace(/\[DAYOFMONTH\]/g,
          now.getDate() + 1).replace(/\[DAYOFYEAR\]/g, dayOfTheYear).replace(
          /\[MONTH\]/g, now.getMonth() + 1).replace(/\[MINUTE\]/g,
          now.getMinutes()).replace(/\[HOUR\]/g, now.getHours());

      var ext = path.extname(fname);
      if (ext.length)
        fname = fname.substr(0, fname.length - (ext.length + 1));

      fname += "-" + Date.now();

      if (ext.length)
        fname += ext;

      // write test
      if (!fs.existsSync(fname)) {
        fs.writeFileSync(fname, '');
        try {
          fs.unlinkSync(fname);
        } catch (ex) {
        }
      } else {
        fs.appendFileSync(fname, '');
      }
      log("Using log_path: " + fname);
      return fname;
    } catch (ex) {
      if (!firstError) {
        firstError = ex.message;
      }
      log("Cannot use file " + fname + " for logging. Error: " + ex.message);
    }
  }

  // if arrived here, it means, that he couldn't find any valid log_path
  monLogDisabled = true;
  log("Cannot use logging feature: " + firstError, true);
};

// ************************ Here starts the main logic

/**
 * Makes maxAttemptCount attempts for sending specific http request to the
 * monitor
 * 
 * @param urlPath
 *          {string} - this is the last part of the url (http://host:port/path)
 * @param json
 *          {object} - this object (stringified) will be sent by POST method
 * @param expectedAnswer
 *          {string} - The answer from monitor is compared with this value. If
 *          they are equal, sendRequest is considered to be successful
 * @param func
 *          {function} - this function should be called on next attempt
 * @param env
 *          {object} - this object should have 3 members { cb, errMsg, okMsg }.
 *          cb will be invoked after sendRequest completes and will receive 2
 *          params (err true/false, txt). If error occurs, txt will have error
 *          message, otherwise, txt will have returned msg.
 */
var sendDataInLoop = function(urlPath, json, expectedAnswer, func, env) {

  if (attemptID++ >= maxAttemptCount) {
    var s = "Cannot sent data to the monitor after " + attemptID
        + " attempts. Giving up.";
    if (env) {
      if (env.errMsg) {
        s = env.errMsg + " " + s;
      }
      if (env.cb) {
        env.cb(true, s);
      }
    }
    logError(s);
    return;
  }

  sendRequest(
      urlPath,
      json,
      expectedAnswer,
      function(err, msg) {
        if (err) {
          var _exit = !iAmTheMonitor && msg
              && msg.toString().indexOf(strings.command_kill) !== -1;
          if (msg)
            msg = msg + "";
          if (msg && msg.indexOf(strings.command_refuse) > -1 && env && env.cb) {
            var s = msg.replace(strings.command_refuse, "");
            if (env.errMsg) {
              s = env.errMsg + " " + s;
            }

            if (_exit) {
              s = "Monitor says, that the app was already killed and needs to be terminated. Exiting now.";
              // don't exit yet. Give a chance to invoke user's callback
            }

            env.cb(true, s);
          } else {
            setTimeout(function() {
              func(env.cb);
            }, attemptInterval);
          }

          // exit now
          if (_exit)
            process.exit();
        } else {
          var s = "Successfully sent data to the monitor on attempt no "
              + attemptID + ". " + monitorUrl + "/" + urlPath;

          if (env) {
            if (env.okMsgEx && msg !== expectedAnswer) {
              s = env.okMsgEx.replace("__body__", msg.replace(expectedAnswer,
                  ""));
            } else if (env.okMsg) {
              s = env.okMsg.replace("__body__", msg);
            }
            if (env.cb) {
              env.cb(false, s);
            }
          }
          log(s);
        }
      });
};

var subscribe = function(cb) {
  var _path;
  if (process.isPackaged) {
    _path = path.resolve(process.argv[0]);
  } else {
    var len = process.argv.length;
    var fst = process.argv[1].toLowerCase(), scd = (len > 2) ? process.argv[2]
        .toLowerCase() : "";
    if (fst == 'monitor' && scd == 'run' && len > 3) {
      _path = path.resolve(process.argv[3]);
    } else if (fst == 'mt' || fst == 'mt-keep' || fst.indexOf('mt-keep:') >= 0
        || fst.indexOf('mt:') >= 0) {
      _path = path.resolve(process.argv[2]);
    } else
      _path = path.resolve(process.argv[1]);
  }

  var json = {
    pid : process.pid,
    path : _path,
    argv : process.argv,
    config : appConfig,
    threadIDs : [ process.threadId ],
    isPackaged : process.isPackaged,
    old_pid : process.env[argNames.monitor_old_pid],
    respawn_id : process.env[argNames.monitor_respawn_id]
  };

  if (process.env.JX_MONITOR_RUN)
    json.JX_MONITOR_RUN = process.env.JX_MONITOR_RUN;

  var env = {
    cb : cb,
    errMsg : "Cannot subscribe process " + process.pid + " to the monitor.",
    okMsg : "Successfully subscribed process " + process.pid
        + " to the monitor.",
    okMsgEx : "__body__."
  };

  sendDataInLoop(strings.send_data, json, strings.get_dataOK, subscribe, env);
};

var unsubscribe = function(cb) {
  var cmd = {
    pid : process.pid
  };
  cmd[strings.send_command_hash] = argNames.leave_me;
  var env = {
    cb : cb,
    errMsg : "Cannot unsubscribe process " + process.pid + " from the monitor.",
    okMsg : "Successfully unsubscribed process " + process.pid
        + " from the monitor."
  };
  sendDataInLoop(strings.send_command, cmd, strings.get_dataOK, unsubscribe,
      env);
};

var killPath = function(_path, cb) {
  var cmd = {
    path : _path
  };
  if (process.getuid) {
    cmd.uid = process.getuid();
  }
  cmd[strings.send_command_hash] = argNames.leave_me;
  var env = {
    cb : cb,
    errMsg : "Cannot kill process with path " + _path + ".",
    okMsg : "Process with given path is killed: " + _path + ".",
    okMsgEx : "__body__."
  };
  sendDataInLoop(strings.send_command, cmd, strings.get_dataOK, killPath, env);
};

var stopServer = function(cb) {
  var cmd = {};
  if (process.getuid) {
    cmd.uid = process.getuid();
  }
  cmd[strings.send_command_hash] = argNames.stop_monitor;
  var env = {
    cb : cb,
    errMsg : "Cannot stop the monitor process.",
    okMsg : "Monitor confirmed receiving the stop request.\n__body__\nMonitor stopped."
  };
  sendDataInLoop(strings.send_command, cmd, strings.monitor_shutdown,
      stopServer, env);
};

/**
 * Checks single monitored process. If does not exists - respawns it.
 * 
 * @param json
 *          {object} - This is an object sent from the client. It must have a
 *          json.pid field.
 * @param respawn
 *          {boolean}
 */
var checkProcess = function(json) {

  if (json.respawning || json.remove)
    return;

  // sending kill(0) check if process exists. Should work on all platforms
  var isAlive = false;
  try {
    isAlive = process.kill(json.pid, 0);
  } catch (ex) {
  }
  if (!isAlive) {
    if (json.respawnAttemptID++ > maxAttemptCount) {
      logError("Cannot spawn the process after " + json.respawnAttemptID
          + " attempts. Giving up.");
      delete monitoredProcesses[json.pid];
      return;
    }

    var _env = JSON.parse(JSON.stringify(process.env));
    _env[argNames.monitor_old_pid] = json.pid;
    _env[argNames.monitor_respawn_id] = (json.respawn_id || 0) + 1;
    if (json.JX_MONITOR_RUN)
      _env.JX_MONITOR_RUN = json.JX_MONITOR_RUN;

    var child = require("child_process").spawn(json.argv[0],
        json.argv.slice(1), {
          env : _env
        });

    if (child) {
      monitoredProcesses[json.pid].respawning = true;
      monitoredProcesses[json.pid].child = child;
      log("Respawned process. Old pid : " + json.pid + " with args "
          + JSON.stringify(json.argv));
    }
  }
};

/**
 * Checks for every monitored process, if it still exists.
 */
var checkProcesses = function() {
  if (!monitoredProcesses || massiveKiller || childrenCheckInProgress)
    return;
  if (childrenStopInProgress) {
    // try again in a moment
    setTimeout(checkProcesses, 200);
    return;
  }
  childrenCheckInProgress = true;

  var remove = [];
  for ( var pid in monitoredProcesses) {
    if (monitoredProcesses.hasOwnProperty(pid)) {
      if (monitoredProcesses[pid].remove) {
        remove.push(pid);
        continue;
      }
      checkProcess(monitoredProcesses[pid]);
    }
  }

  // removes entries marked for removal
  for (var a = 0, len = remove.length; a < len; a++)
    delete monitoredProcesses[remove[a]];

  // cleanup for killedProcesses
  var now = new Date();
  for ( var pid in killedProcesses) {
    if (killedProcesses.hasOwnProperty(pid)) {
      if (now - killedProcesses[pid] > monConfig.monitor.start_delay * 2)
        delete killedProcesses[pid];
    }
  }

  childrenCheckInProgress = false;
  setTimeout(checkProcesses, monConfig.monitor.check_interval);
};

/**
 * Makes few attempts to stop all monitored children
 * 
 * @param cb
 */
var stopChildren = function(cb, arr, env) {
  if (!cb)
    return;

  if (!arr)
    massiveKiller = true;
  var _arr = (!arr) ? monitoredProcesses : arr;

  if (!monitoredProcesses) {
    env.msg = "There are no children currently monitored, so there is nothing to kill.";
    cb(env);
    return;
  }

  if (childrenCheckInProgress) {
    // try again in a moment
    log("try again in a moment");
    setTimeout(function() {
      stopChildren(cb, arr, env);
    }, 200);
    return;
  }
  childrenStopInProgress = true;

  if (!env) {
    env = {
      killed : [],
      killFailed : [],
      report : []
    };
  }

  var cnt = 0;
  if (massiveKiller)
    env.report.push(strings.monitor_shutdown);

  for ( var pid in _arr) {
    if (!_arr.hasOwnProperty(pid))
      continue;

    cnt++;

    if (_arr[pid].remove)
      continue;

    _arr[pid].killAttempt = _arr[pid].killAttempt || 0;
    if (_arr[pid].killAttempt < maxAttemptCount) {

      var pid_to_kill = _arr[pid].respawning && _arr[pid].child ? _arr[pid].child.pid
          : pid;
      var isAlive = false;
      try {
        isAlive = process.kill(pid_to_kill, 0);
      } catch (ex) {
      }
      if (isAlive) {
        _arr[pid].killAttempt++;
        process.kill(pid_to_kill);
      } else {
        _arr[pid].killed = true;
        _arr[pid].remove = true;
        env.report.push("CLOSED -> " + _arr[pid].path + " (" + pid + ")");
        env.killed.push(pid);

        // TODO: better remove after the loop finishes
        delete monitoredProcesses[pid];
        killedProcesses[pid] = new Date();
        continue;
      }
    } else {
      _arr[pid].cannotKill = true;
      env.report.push("NOT CLOSED (after " + _arr[pid].killAttempt
          + " attempts) -> " + _arr[pid].path);
      env.killFailed.push(pid);
    }
  }

  if (typeof env.total === "undefined")
    env.total = cnt;

  if (env.total != env.killed.length + env.killFailed.length) {
    // log("again, " + cnt + ", " + env.killed.length + ", " +
    // env.killFailed.length);
    setTimeout(function() {
      stopChildren(cb, arr, env);
    }, 200);
  } else {
    if (env.total == 0) {
      env.report
          .push("There are no children currently monitored, so there is nothing to kill.");
    } else {
      env.report = [ "Initiating " + env.total + " children kill:" ]
          .concat(env.report);
    }
    // preparing return message
    var killed = env.killed.length;
    var failed = env.killFailed.length;
    if (!failed) {
      env.msg = strings.get_dataOK;
      if (massiveKiller)
        env.msg += strings.monitor_shutdown;
      else
        env.msg += killed === 1 ? "The process with given path was killed"
            : "All " + killed + " processes with given path were killed";
    } else {
      env.msg = strings.command_refuse
          + "\nSome of the processes could not be killed: "
          + env.killFailed.join(", ");
    }

    var report = env.report.join("\n");
    log(report);
    cb(env);
    childrenStopInProgress = false;
  }
};

/**
 * Checks, whether the command was made by the same user, which started the
 * monitor.
 * 
 * @param cmd
 * @param json
 * @param res
 * @return {boolean} - true, if command is allowed to execute, false if was
 *         refused
 */
var checkUID = function(cmd, json, res) {

  // no uid checking for windows
  if (isWindows) {
    return true;
  }

  var error = null;

  if (typeof json.uid === "undefined") {
    error = "Uid was not provided with command " + cmd;
  } else {
    var uid = parseInt(json.uid);
    if (isNaN(uid)) {
      error = "Wrong uid (" + json.uid + ") provided with command " + cmd;
    } else {

      var users = monConfig.monitor.users;
      if (users) {
        if (!util.isArray(users)) {
          error = "monitor.users in config file is expected to be an array.";
        } else {
          if (!monConfig.monitor.uids) {
            // monitor.uids - dynamically evaluated uid from user names
            monConfig.monitor.uids = {};
            for (var a = 0, len = users.length; a < len; a++) {
              var ret = jxcore.utils.cmdSync("id -u " + users[a]);
              var _uid = parseInt(ret.out);
              if (isNaN(_uid)) {
                log("Cannot get uid of a user " + users[a] + ": "
                    + ret.out.toString().trim());
              } else {
                monConfig.monitor.uids[_uid] = true;
              }
            }
          }
        }
      }

      var uids = monConfig.monitor.uids;
      if ((uids && uids[uid]) || process.getuid() === uid || uid === 0) {
        // force root can close
        return true;
      } else {
        error = "Refused to run the command since it is made by a different user.";
      }
    }
  }

  if (error) {
    log(error);
    res.writeHead(200, {
      'Content-Type' : 'text/html'
    });
    res.end(error + strings.command_refuse);
    return false;
  } else {
    return true;
  }
};

/**
 * Start the http server, which will serve as monitor.
 */
var startServer = function() {

  var __server = function(req, res) {

    if (exitting)
      return;

    var req_question = req.url == "/" + strings.send_question;
    var req_data = req.method == 'POST' && req.url == "/" + strings.send_data;
    var req_command = req.method == 'POST'
        && req.url == "/" + strings.send_command;
    var req_json = req.url.slice(0, 5) == "/json";
    var req_logs = req.url.slice(0, 5) == "/logs";
    var parsed = _url.parse(req.url, true);

    // for example /json?silent=true would not save to log the fact, that the
    // request was received
    var silent = (parsed.query && typeof parsed.query.silent !== "undefined")
        || req.url.indexOf("favicon.ico") > -1;
    // for example /logs?html returns html formatted logs (with <br> instead of
    // \n)
    var _html = parsed.query && typeof parsed.query.html !== "undefined";

    var htmlize = function(str) {
      if (!_html)
        return str;

      return str.replace(new RegExp("\n", 'g'), '<br>').replace(
          new RegExp("    ", 'g'), "&nbsp;&nbsp;&nbsp;");
    };

    if (!silent) {
      log("Monitor received request: " + req.url);
    }

    if (req_question) {
      res.writeHead(200, {
        'Content-Type' : 'text/html'
      });
      res.end(strings.get_answer);
      return;
    }

    // here monitor is receiving data about process sent from that process
    if (req_data) {
      var body = '';

      req.on('data', function(data) {
        body += data;
      });

      req
          .on(
              'end',
              function() {
                if (!body) {
                  return;
                }

                res.writeHead(200, {
                  'Content-Type' : 'text/html'
                });

                try {
                  var json = JSON.parse(body.toString());
                  json.respawnAttemptID = 0;
                  json.old_pid = getInt(json.old_pid);

                  if (!monitoredProcesses[json.pid]) {
                    if (json.old_pid
                        && (killedProcesses[json.old_pid] || !monitoredProcesses[json.old_pid])) {
                      // this is the case, when app subscribes back with a new
                      // pid,
                      // but the old pid is not monitored any more (or was
                      // already killed)
                      log("Could not find the app with the old PID: "
                          + json.old_pid);
                      res.end(strings.command_refuse + "|"
                          + strings.command_kill);
                      return;
                    }

                    if (json.old_pid) {
                      // app comes back after being restarted
                      log("Found app with the old PID: " + json.old_pid);
                      json["respawn_id"] = getInt(json.respawn_id);
                      monitoredProcesses[json.pid] = json;
                      // mark old for removal
                      monitoredProcesses[json.old_pid].remove = true;
                    } else {
                      // app send data to monitor for the first time
                      // (subscription)
                      json["respawn_id"] = 0;
                      monitoredProcesses[json.pid] = json;
                    }
                  } else {
                    // just another request of monitored app
                    monitoredProcesses[json.pid].threadIDs
                        .push(json.threadIDs[0]);
                  }
                  res.end(strings.get_dataOK);
                  setTimeout(checkProcesses, monConfig.monitor.check_interval);
                  log("Received data from process. " + body);
                } catch (ex) {
                  res.end("That's not it.");
                  logError("That's not it." + ex);
                }
              });
      return;
    }

    if (req_json) {
      var arr = [];
      for ( var pid in monitoredProcesses) {
        if (monitoredProcesses.hasOwnProperty(pid)
            && !monitoredProcesses[pid].remove)
          arr.push(monitoredProcesses[pid]);
      }
      var ret = JSON.stringify(arr, function(k, v) {
        return k === "child" ? undefined : v;
      }, 4);
      arr = null;
      res.writeHead(200, {
        'Content-Type' : 'text/html'
      });
      res.end(htmlize(ret));
      ret = null;
      return;
    }

    if (req_logs) {
      if (fs.existsSync(monLogPath)) {
        try {
          ret = fs.readFileSync(monLogPath).toString();
        } catch (ex) {
          ret = "Cannot read log file from " + monLogPath + "<br>. Exception: "
              + ex.message;
        }
      } else {
        ret = "The log file does not exist: " + monLogPath;
      }
      res.writeHead(200, {
        'Content-Type' : 'text/html'
      });
      res.end(htmlize(ret));
      ret = null;
      return;
    }

    if (req_command) {

      var body = '';

      req.on('data', function(data) {
        body += data;
      });

      req.on('end', function() {
        if (!body)
          return;

        try {
          var json = JSON.parse(body.toString());

          var cmd = json[strings.send_command_hash] || null;

          if (cmd == argNames.stop_monitor) {
            if (!checkUID(cmd, json, res))
              return;

            stopChildren(function(env) {

              res.on('finish', function() {
                // give a little bit more time for client to receive the message
                setTimeout(process.exit, 200);
              });
              res.writeHead(200, {
                'Content-Type' : 'text/html'
              });
              res.end(env.msg);
              monitoredProcesses = null;
              log("Monitor received the stop request. Exiting.");
              exitting = true;
              // make sure it exits anyway
              setTimeout(process.exit, 1000).unref();
            });
            return;
          }

          if (cmd == argNames.leave_me && json.pid) {
            var s = "Monitor received request from pid " + json.pid
                + " to stop its monitoring";
            if (monitoredProcesses[json.pid]) {
              delete monitoredProcesses[json.pid];
            } else {
              s += ", but this pid is not monitored. Request ignored";
            }
            log(s + ".");
            res.writeHead(200, {
              'Content-Type' : 'text/html'
            });
            res.end(strings.get_dataOK);
            return;
          }

          if (cmd == argNames.leave_me && json.path) {
            if (!checkUID(cmd, json, res)) {
              return;
            }

            var s = "Monitor received request for " + json.path
                + " to get killed";
            var arr = {};
            var ln = 0;
            for ( var o in monitoredProcesses) {
              var proc = monitoredProcesses[o];
              if (monitoredProcesses.hasOwnProperty(o) && proc
                  && proc.path == json.path && !proc.remove) {

                // try to kill immediately
                if (proc.child) {
                  try {
                    proc.child.kill();
                    // we don't care whether it succeeded or not
                    // stopChildren will check it out
                  } catch (ex) {
                  }
                }

                proc.killAttempt = 0;
                arr[o] = proc;
                ln++;
              }
            }

            if (ln === 0) {
              s += ", but this path is not monitored. Request ignored";
              log(s + ".");

              res.writeHead(200, {
                'Content-Type' : 'text/html'
              });
              res.end(strings.get_dataOK
                  + "This path is not monitored. Request ignored");
            } else {
              log(s + ".");
              stopChildren(function(env) {
                res.writeHead(200, {
                  'Content-Type' : 'text/html'
                });
                res.end(env.msg);
              }, arr);
            }
            return;
          }

          // heartbeat by main monitor's server
          if (cmd == argNames.checkPath && json.path)
            return checkHeartbeat(res, json);

        } catch (ex) {
          log("Error while processing the request: " + ex);
          res.end("That's not it.");
        }
      });
      return;
    }

    // if arrived here, just return nothing
    res.end();
  };

  if (!https_keys) {
    http = _http;
    http.createServer(__server).listen(monConfig.monitor.port,
        monConfig.monitor.host);
  } else {
    http = _https;
    var https_options = {
      key : fs.readFileSync(https_keys.httpsKeyLocation),
      cert : fs.readFileSync(https_keys.httpsCertLocation)
    };
    http.createServer(https_options, __server).listen(monConfig.monitor.port,
        monConfig.monitor.host);
  }

  process.on("uncaughtException", function(ex) {
    log("Monitor process received uncaughtException: " + ex + ".", true);
    log(ex.stack.toString(), true);
  });

  process.on("exit", function(code) {
    log("Monitor process is exiting with code " + code + ".");
  });

  log("JXcore monitoring is started.", true);
  log("Monitor started on " + monitorUrl);
  iAmTheMonitor = true;
  initRemoteAccess(monConfig);
};

// ************************ monitor called from command line with: stop or
// restarting

/**
 * Send to the monitor request to stop, and eventually starts it again
 * afterwards.
 * 
 * @param restart
 *          {boolean} - If true, the current process will create new http server
 *          to serve as a monitor.
 */
var stopOrRestart = function(cb, restart) {

  isMonitorOnline(function(err, txt) {
    if (!err) {
      attemptID = 0;

      if (!restart) {
        stopServer(cb);
      } else {
        stopServer(function(err, txt) {
          if (err) {
            if (cb) {
              cb(true, txt);
            } else {
              throwError(txt);
            }
          } else {
            // let's give time for previous monitor process to exit
            setTimeout(function() {
              if (cb) {
                cb(false, txt);
              }
              startServer();
            }, 1000);
          }
        });
      }
    } else {
      var cmd = parsedArgv[argNames.stop_monitor] ? argNames.stop_monitor
          : argNames.restart_monitor;
      var s = "Monitor is not running. Cannot execute '" + cmd + "' command.";
      if (cb) {
        cb(true, s);
        logError(s);
      } else {
        throwError(s);
      }
    }
  });
};

// ************************ ************************ ************************
// ************************ exported methods for monitor process
// ************************ ************************ ************************

/**
 * Tries to connect to a monitor in order to check, if it's online.
 * 
 * @param cb
 *          {function} - Callback will receive 2 params (err true/false, txt).
 */
exports.checkMonitorExists = function(cb) {
  monLogPath = findLogPath();
  isMonitorOnline(cb);
};

/**
 * Start the http server, which will serve as monitor,
 * 
 * @param cb
 *          {function} - Callback is optional. When provided will receive 2
 *          params (err true/false, txt).
 */
exports.startMonitor = function(cb) {
  monLogPath = findLogPath();
  startServer();
};

/**
 * Start the http server, which will serve as monitor, but only if no monitor is
 * already running.
 * 
 * @param cb
 *          {function} - Callback is optional. When provided will receive 2
 *          params (err true/false, txt).
 */
exports.startIfNotExists = function(cb) {

  monLogPath = findLogPath();
  isMonitorOnline(function(err, txt) {
    if (!err) {
      var s = "Another monitor is already running";
      if (cb) {
        cb(true, s);
        logError(s);
      } else {
        throwError(s);
      }
    } else {
      startServer();
    }
  });

};

/**
 * Send to the monitor request to stop itself.
 * 
 * @param cb
 *          {function} - Callback is optional. When provided will receive 2
 *          params (err true/false, txt).
 */
exports.stopMonitor = function(cb) {
  monLogPath = findLogPath();
  stopOrRestart(cb, false);
};

/**
 * Send to the monitor request to stop itself, and then this (current) process
 * will create new http server to serve as a monitor.
 * 
 * @param cb
 *          {function} - Callback is optional. When provided will receive 2
 *          params (err true/false, txt).
 */
exports.restartMonitor = function(cb) {
  monLogPath = findLogPath();
  stopOrRestart(cb, true);
};

// ************************ ************************ ************************
// ************************ exported methods for monitored app process
// ************************ ************************ ************************

var checkForSID_MT = function(sid) {
  var exists = false;

  if (jxcore.store.shared.safeBlock) {
    jxcore.store.shared.safeBlock(sid, function() {
      exists = jxcore.store.shared.exists(sid);
      if (!exists) {
        jxcore.store.shared.set(sid, "some_value");
      }
    });
  } // for win, because safeBlock is not yet implemented
  else {
    exists = jxcore.store.shared.exists(sid);
    if (!exists) {
      jxcore.store.shared.set(sid, "some_value");
    }
  }

  return exists;
};

var subscribeSID = "_jx_monitor_subscribe_by_thread";
var unsubscribeSID = "_jx_monitor_UNsubscribe_by_thread";

/**
 * Send to the monitor request to subscribe process.pid for monitoring. If
 * monitor is not online, throws an error. This method is intended to be called
 * from an app.
 * 
 * @param cb
 *          {function} - Callback is optional. When provided will receive 2
 *          params (err true/false, txt).
 */
exports.followMe = function(cb, waitcb) {
  // do not write to log on app side
  monLogDisabled = true;

  if (checkForSID_MT(subscribeSID)) {
    if (cb) {
      cb(true,
          "Another subthread already is trying to subscribe to the monitor.");
    }
    return;
  }

  isMonitorOnline(function(err, txt) {

    if (!err) {
      attemptID = 0;

      var delay = monConfig.monitor.start_delay || 0;

      if (delay) {
        if (waitcb) {
          waitcb(delay);
        }
        setTimeout(function() {
          subscribe(cb);
        }, delay).unref();
      } else {
        subscribe(cb);
      }
    } else {
      if (cb) {
        cb(true, "Cannot subscribe to the monitor. " + txt);
      }
    }
  });

  // allow to unsubscribe again
  jxcore.store.shared.remove(unsubscribeSID);
};

/**
 * Send to the monitor request to unsubscribe process.pid from monitoring. If
 * monitor is not online, throws an error. * This method is intended to be
 * called from an app.
 * 
 * @param cb
 *          {function} - Callback is optional. When provided will receive 2
 *          params (err true/false, txt).
 */
exports.leaveMe = function(cb) {
  // do not write to log on app side
  monLogDisabled = true;

  if (checkForSID_MT(unsubscribeSID)) {
    if (cb) {
      cb(true,
          "Another subthread already is trying to unsubscribe from the monitor.");
    }
    return;
  }

  isMonitorOnline(function(err, txt) {
    if (!err) {
      attemptID = 0;
      unsubscribe(cb);
    } else {
      if (cb) {
        cb(true, "Cannot unsubscribe from the monitor. " + txt);
      }
    }
  });

  // allow to subscribe again
  jxcore.store.shared.remove(subscribeSID);
};

exports.releasePath = function(path, cb) {
  // do not write to log on app side
  monLogDisabled = true;

  isMonitorOnline(function(err, txt) {
    if (!err) {
      attemptID = 0;
      killPath(path, cb);
    } else {
      if (cb) {
        cb(true, "Cannot kill the given path. " + txt);
      }
    }
  });
};

exports.checkPath = function(appPath, options, cb) {

  if (typeof options === "function") {
    cb = options;
    options = {};
  }

  if (!cb || typeof cb !== "function")
    throw new Error("Callback required for jxcore.monitor.check().");

  if (!options)
    options = {};

  var cfg = monConfig.monitor;
  // jxcore.utils.console.info(cfg);

  var options = {
    host : options.host || cfg.host,
    port : options.port || cfg.port,
    path : "/" + strings.send_command,
    method : 'POST',
    rejectUnauthorized : false
  };

  var json = {
    path : appPath,
    pass_phrase : options.pass_phrase
  };
  json[strings.send_command_hash] = argNames.checkPath;

  var req = _https.request(options, function(res) {

    var response = "";
    res.on('data', function(d) {
      response += d;
    });

    res.on('end', function() {
      req.connection.destroy();
      cb(false, response);
    });
  });
  req.end(JSON.stringify(json));

  req.on('error', function(e) {
    req.connection.destroy();
    cb(true, e);
  });

};

/**
 * Check validity of remote access configuration and creates a dedicated https
 * server for that
 * 
 * @param monConfig
 * @return {*}
 */
var initRemoteAccess = function(monConfig) {
  var cfg = monConfig.monitor;
  var ra = monConfig.monitor.remote;

  // remote access config is not obligatory
  if (!ra)
    return;

  var _logError = function(txt) {
    var cl = jxcore.utils.console.setColor;
    log(cl("The remote access cannot be activated:\n", "red")
        + cl(txt, "magenta"));
  };

  if (!ra.https)
    return _logError("This feature can work only through HTTPS, "
        + "but the certificates paths are not provided in jx.config.\n");

  var options = checkCertFiles(ra.https);
  if (typeof options === "string")
    return _logError(options);

  if (ra.port === cfg.port)
    return _logError("This feature needs to use separate port than the monitor.");

  if (!ra.port)
    ra.port = cfg.port + 1;

  ra.urlString = ra.host ? "https://" + ra.host + ":" + ra.port
      : "on HTTPS, on all IP addresses and port " + ra.port;

  var srv = _https.createServer(options, function(req, res) {

    if (exitting)
      return;

    var req_command = req.method == 'POST'
        && req.url == "/" + strings.send_command;
    if (req_command) {
      var body = '';

      req.on('data', function(data) {
        body += data;
      });

      req.on('end', function() {
        if (!body)
          return;

        try {
          var json = JSON.parse(body.toString());
          var cmd = json[strings.send_command_hash] || null;

          // heartbeat by dedicated monitor's server
          if (cmd == argNames.checkPath && json.path)
            return checkHeartbeat(res, json);

        } catch (ex) {
          log("Error for remote access call:" + ex);
        }
      });
      return;
    }

    log("Remote access invalid call: " + req.url);
  });

  if (ra.host)
    srv.listen(ra.port, ra.host);
  else
    srv.listen(ra.port);

  log(jxcore.utils.console.setColor("Remote access enabled", ra.urlString,
      "green"), true);
  return ra;
};

/**
 * Check validity of https certificate files from config file
 * 
 * @param obj
 * @return {object} Returns error string or loaded certificate files (if
 *         everything is ok)
 */
var checkCertFiles = function(obj) {

  // od param names support
  if (!obj.key && obj.httpsKeyLocation)
    obj.key = obj.httpsKeyLocation;

  if (!obj.cert && obj.httpsCertLocation)
    obj.cert = obj.httpsCertLocation;

  var options = {};
  var arr = [ "key", "cert" ];
  for (var o = 0, len = arr.length; o < len; o++) {
    var _name = arr[o];
    if (!obj[_name])
      return "The `" + _name + "` definition not found.";

    if (!fs.existsSync(obj[_name]))
      return "The `" + _name + "` (" + obj[_name] + ") file not found.";

    try {
      options[_name] = fs.readFileSync(obj[_name]);
    } catch (ex) {
      return "Cannot read the `" + _name + "` (" + obj[_name] + ") file.";
    }
  }

  return options;
};

var checkHeartbeat = function(res, json) {

  var cnt = 0;
  for ( var o in monitoredProcesses) {
    if (monitoredProcesses.hasOwnProperty(o)
        && monitoredProcesses[o].path == json.path) {
      cnt++;
    }
  }
  res.writeHead(200, {
    'Content-Type' : 'text/html'
  });
  res.end(cnt + "");
};

var getInt = function(val, defaultVal) {
  if (!defaultVal)
    defaultVal = 0;
  val = parseInt(val);
  return isNaN(val) ? defaultVal : val;
};

exports.respawned = getInt(process.env[argNames.monitor_respawn_id]);