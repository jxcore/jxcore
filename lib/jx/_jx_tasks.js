// Copyright & License details are available under JXCORE_LICENSE file

var uw = process.binding('thread_wrap');

var markers = Object.create(null);

var events = {
  'emptyQueue': [],
  'message': []
};

var trackerId = 0;
var cpuCount = 2;
var cpuSet = false;
var exiting = false;

exports.setThreadCount = function(count) {
  if (process.subThread) {
    throw new Error(
        'You can not change the thread count under a subthread.');
  }
  // TODO(obastemur) use commons.h MAX_JX_THREADS
  if (count < 2 || count > 64) {
    throw new Error(
        'setThreadCount - min 2, max 64');
  }

  if (!process.__tasking) {
    cpuSet = true;
    cpuCount = count + 1;
  }
};

exports.killThread = function(threadId, keep_execution) {
  if (threadId < 0 || threadId > 63) {
    throw new RangeError(
        'Thread Id - min 0, max 63');
  }

  uw.killThread(threadId, keep_execution);
};

var sleep = function(timeout) {
  setTimeout(function() {
    jxcore.utils.continue();
  }, timeout);
  jxcore.utils.pause();
};

exports.unloadThreads = function(already_exiting) {
  if (process.subThread) {
    throw new Error(
        'You can not unload the threads under a sub thread. Try killThread.');
  }

  if (already_exiting) {
    // TODO(obastemur) after embedding interface implemented remove this!
    var tcount = uw.threadCount();
    if (tcount == 0) return;

    // give some time
    while (uw.threadCount() < tcount) {
      tcount = uw.threadCount();
      if (tcount == 0)
        return;
      sleep(20);
    }
  }

  exports.runOnce(function() {
    process.unloadThread();
  }, null, true, true);

  if (!already_exiting) {
    var ref_wait = setTimeout(function() {
      // dummy wait
      // TODO(obastemur) after embedding interface implemented remove this!
    }, 200);
    ref_wait.unref();
  }
};

var gcc_alive = false;

exports.begin = function() {
  if (exiting) return;
  if (process.subThread) {
    throw new Error(
        'You can not modify thread pool under a subthread.');
  }

  if (!process.__tasking) {
    process.on('exit', function() {
      exiting = true;
      exports.unloadThreads(1);
      uw.setProcessExiting(false);
    });

    // TODO(obastemur) get the number from commons.h MAX_JX_THREADS
    if (cpuCount > 64) {
      cpuCount = 64;
    }
    uw.setCPUCount(cpuCount);
    process.__tasking = true;
  }
};

exports.jobCount = function() {
  return uw.jobsCount();
};

exports.forceGC = function() {
  uw.freeGC();
};

var dummy = function() {
  if (exiting) return;

  gcc_alive = uw.jobsCount() > 0;

  if (gcc_alive) return;

  if (cinter != null) {
    clearInterval(cinter);
    cinter = null;
  }

  if (trackerId != 0) {
    trackerId = 0;
    cbb = 0;
    markers = {};
    if (events['emptyQueue'].length) {
      var arr = events['emptyQueue'];
      for (var o in arr) {
        if (o != null && arr.hasOwnProperty(o)) arr[o]();
      }
    }
  }
};

var cinter = null;
var cbb = 0;

var runCinter = function() {
  if (cinter != null || exiting) return;
  cinter = setInterval(function() {
    if (cbb >= trackerId) {
      dummy();
    }
  }, 250);
};

var emitMessage = function(obj) {
  var t = (obj.threadId || obj.threadId === 0) ? obj.threadId : obj.tid;
  var d = (obj.params || obj.params === 0) ? obj.params : obj.data;
  exports.emit('message', t, d);
};

var gcc = function(arr) {
  if (exiting) {
    return;
  }

  for (var o in arr) {
    if (o == null || !arr.hasOwnProperty(o)) continue;

    var obj = JSON.parse(arr[o]);

    if (!obj || obj._id == -2)// runOnce
    {
      cbb++;
      if (obj) delete obj;
      continue;
    }

    if (obj.threadId || obj.threadId === 0 || obj.tid || obj.tid === 0) {
      if (obj.resetMe) {
        cbb += obj.counter;

        // give a chance calling method to exit
        cbb--;

        var tot = 500;
        if (obj.threadId > 0) tot = 575 * (obj.threadId);

        var ref_wait = setTimeout(function() {
          if (exiting) return;
          // TODO(obastemur) REPLACE BELOW to new sleep->thread block check
          if (obj)
            uw.resetThread(obj.threadId);

          var ref_counter = setTimeout(function() {
            if (!process._MTED) {
              cbb++;
            }
          }, 1000);
          ref_counter.unref();
        }, tot);
        ref_wait.unref();

      } else if (obj.wait) {
        cbb -= obj.wait;
      } else if (events['message'].length) {
        emitMessage(obj);
      }
      delete obj;
      continue;
    }

    if (obj.reset) {
      delete obj;
      continue;
    }

    var cb = (obj._id || obj._id === 0) ? markers[obj._id] : 0;
    cbb++;
    if (!cb && obj.dummy) {
      delete obj;
      continue;
    }

    if (cb) {
      if (obj.dummy) {
        cb(null);
      } else {
        cb(null, obj.o);
      }
      delete markers[obj._id];
    }
    delete obj;
  }
  delete arr;
};

exports.getThreadCount = function() {
  if (!process.__tasking && !process.subThread) {
    return (cpuSet) ?
        cpuCount - 1 : 2;
  }

  return uw.getCPUCount();
};

exports.on = function(event, cb) {
  if (!event || !event.substr) {
    throw new TypeError('expects event_name as a string');
  }

  var ee = events[event];
  if (ee) {
    ee.push(cb);
  }
};

exports.emit = function(event, p1, p2, p3) {
  if (events[event].length) {
    var zarr = events[event];
    for (var zo in zarr) {
      if (zarr.hasOwnProperty(zo))
        zarr[zo](p1, p2, p3);
    }
  }
};

var taskId = 1;

var isFunction = function(method) {
  return Object.prototype.toString.call(method) == '[object Function]';
};

var _activated = false;

var getMethod = function(method) {
  var mt;
  if (!method) {
    throw new TypeError('jxcore.tasks requires a function body');
  }

  if (!_activated) {
    _activated = true;
  }

  if (method.define || method.logic) {
    if (!method.define ||
        !isFunction(method.define) && !method.define.substr) {
      throw new Error('Task object requires a task.define function.');
    }
    if (!method.logic) {
      method.logic = function() {
      };
    }
    var def = method.define.toString();
    var ind = def.lastIndexOf('return ');
    if (ind > 0) {
      var q = def.indexOf('}', ind);
      var z = def.indexOf('}', q + 1);
      if (z < 0) {
        if (def.indexOf(';', ind) > 0)
          console.error('Warning! task.define can not return!');
      }
    }
    mt = method.logic.toString() + '$$&JX;1$node$&JX;$' + def +
        '$$&JX;2$node$&JX;$' + method.waitLogic;
  } else {
    if (!isFunction(method)) {
      throw new TypeError(
          'jxcore.tasks requires a function body');
    }
    mt = method.toString();
  }

  return mt;
};

exports.addTask = function(method, param, cb) {
  if (param === undefined)
    param = 'null';
  else
    param = JSON.stringify(param);

  if (arguments.length > 3) // pass-through deprecated usage.
    exports._addTask(method, param, cb, arguments[3]);
  else
    exports._addTask(method, param, cb);
};

exports._addTask = function(method, param, cb) {
  var cbId = trackerId, tId;

  if (process.subThread) {
    throw new Error(
        'You can not add a task under a subthread.');
  }
  if (arguments.length > 3) {
    // handle deprecated usage: (method, param, cb, obj).
    var msg = '(jxcore.tasks) warning: Passing obj arguments when ' +
        'creating tasks has been deprecated. See ' +
        'https://github.com/jxcore/jxcore/issues/334#issuecomment-100581210';
    if (process.throwDeprecation)
      throw new Error(msg);
    else if (process.traceDeprecation)
      console.trace(msg);
    else
      console.error(msg);
    if (cb) {
      var obj = arguments[3];
      var depr_cb = cb;
      cb = function(err, result) {
        if (err) throw err;
        depr_cb(obj, result);
      };
    }
  } else if (cb && cb.length === 1) {
    // handle deprecated usage: (method, param, function(result) {...})
    var msg = '(jxcore.tasks) warning: Task callback functions ' +
        'now take two arguments (err, result). See ' +
        'https://github.com/jxcore/jxcore/issues/334';
    if (process.throwDeprecation)
      throw new Error(msg);
    else if (process.traceDeprecation)
      console.trace(msg);
    else
      console.error(msg);
    var depr_cb = cb;
    cb = function(err, result) {
      if (err) throw err;
      depr_cb(result);
    };
  }

  exports.begin();

  if (method._taskId) {
    tId = method._taskId;
    method = null;
  } else {
    method._taskId = taskId;
    tId = method._taskId;
    taskId++;
    method = getMethod(method);
  }

  if (cb) {
    markers[cbId] = cb;
  } else {
    cbId = -1;
  }

  var err = uw.addTask(tId, method, param, cbId, false);

  if (err > 0) {
    throw new Error('Thread creation error. id:' + err);
  }

  trackerId++;

  if (cinter == null) {
    gcc_alive = true;
    runCinter();
  }
};

exports.runOnce = function(method, param, doNotRemember, skip_thread_creation) {
  if (param === undefined)
    param = null;
  else if (param !== null)
    param = JSON.stringify(param);

  exports._runOnce(method, param, doNotRemember, skip_thread_creation);
};

exports._runOnce = function(method, param, doNotRemember, skip_thread_creat) {
  if (process.subThread) {
    throw new Error(
        'You can not add a task under a subthread.');
  }

  if (!skip_thread_creat)
    exports.begin();

  if (method._taskId) {
    throw new Error('This method was already defined');
  }

  if (param === undefined) {
    param = null;
  }

  var mt = getMethod(method);
  method._taskId = taskId;

  if (doNotRemember === undefined) doNotRemember = false;

  var err = uw.addTask(taskId, mt, param, -2, doNotRemember, skip_thread_creat);
  if (err > 0) {
    throw new Error('Thread creation error. id:' + err);
  }

  taskId++;
  trackerId += exports.getThreadCount();

  if (!skip_thread_creat) {
    if (cinter == null) {
      gcc_alive = true;
      runCinter();
    }
  }
};

exports.register = function(method) {
  if (process.subThread) return;

  return exports.runOnce(method, null, false, true);
};

exports.runOnThread = function(threadId, method, param, cb) {
  if (param === undefined)
    param = 'null';
  else
    param = JSON.stringify(param);

  if (arguments.length > 4) // pass-through deprecated usage.
    exports._runOnThread(threadId, method, param, cb, arguments[4]);
  else
    exports._runOnThread(threadId, method, param, cb);
};

exports._runOnThread = function(threadId, method, param, cb) {
  if (threadId >= cpuCount) {
    throw new Error(
        'Given threadId does not exist. ' +
        'You can change the number of threads from setThreadCount');
  }

  if (process.subThread) {
    throw new Error(
        'You can not add a task under a subthread.');
  }

  if (arguments.length > 4) {
    // handle deprecated usage: (threadId, method, param, cb, obj).
    var msg = '(jxcore.tasks) warning: Passing obj arguments when ' +
        'creating tasks has been deprecated. See ' +
        'https://github.com/jxcore/jxcore/issues/334#issuecomment-100581210';
    if (process.throwDeprecation)
      throw new Error(msg);
    else if (process.traceDeprecation)
      console.trace(msg);
    else
      console.error(msg);
    if (cb) {
      var obj = arguments[4];
      var depr_cb = cb;
      cb = function(err, result) {
        if (err) throw err;
        depr_cb(obj, result);
      };
    }
  } else if (cb && cb.length === 1) {
    // handle deprecated usage: (threadId, method, param, function(result)
    // {...})
    var msg = '(jxcore.tasks) warning: Task callback functions ' +
        'now take two arguments (err, result). See ' +
        'https://github.com/jxcore/jxcore/issues/334';
    if (process.throwDeprecation)
      throw new Error(msg);
    else if (process.traceDeprecation)
      console.trace(msg);
    else
      console.error(msg);
    var depr_cb = cb;
    cb = function(err, result) {
      if (err) throw err;
      depr_cb(result);
    };
  }

  exports.begin();

  var cbId = trackerId;
  if (cb) {
    markers[cbId] = cb;
  } else {
    cbId = -1;
  }

  var mt = getMethod(method);

  process.sendToThread(threadId, {
    '$runIn': 1,
    'mt': mt,
    'param': param,
    'id': taskId,
    'cbId': cbId
  });
  taskId++;
  trackerId++;

  if (cinter == null) {
    gcc_alive = true;
    runCinter();
  }
};

if (!process.subThread) {
  process.on('threadMessage', function(rs) {
    gcc(rs);
  });
}
