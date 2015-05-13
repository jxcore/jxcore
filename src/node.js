// Copyright & License details are available under JXCORE_LICENSE file

(function (process) {
  this.global = this;
  
  if (!Error.captureStackTrace) {
    var _stackProto = function (msg, fileName, lineNumber, columnNumber) {
      if (fileName.indexOf('@') >= 0) {
        var spl = fileName.split('@');
        this._fileName = spl[1];
        this._functionName = spl[0];
      } else {
        this._fileName = fileName;
        this._functionName = "unknown";
      }

      this._lineNumber = lineNumber;
      this._columnNumber = columnNumber;
      this._msg = msg;
    };

    _stackProto.prototype.getFileName = function () {
      return this._fileName;
    };
    _stackProto.prototype.getColumnNumber = function () {
      return this._columnNumber;
    };
    _stackProto.prototype.getLineNumber = function () {
      return this._lineNumber;
    };
    _stackProto.prototype.toString = function () {
      return this._msg
    };
    _stackProto.prototype.isEval = function () {
      // TODO(obastemur) fix this!
      return false;
    };
    _stackProto.prototype.getFunctionName = function () {
      return this._functionName;
    };

    Error.captureStackTrace = function (err, __) {
      var st;
      if (!err.stack) {
        // TODO(obastemur) there must be a better way to do this
        try {
          throw new Error("");
        } catch (e) {
          err.stack = e.stack;
          err.fileName = e.fileName;
          err.lineNumber = e.lineNumber;
          err.columnNumber = e.columnNumber;
        }

        st = err.stack.split('\n');
        st.shift();
      } else {
        if (!Array.isArray(err.stack)) st = err.stack.split('\n');
      }

      if (Array.isArray(err.stack) && err.stack[0] && err.stack[0]._msg) {
        // TODO(obastemur) bug ? app gives back an object that was already processed
      } else {
        err.stack = new Array();
        var stackLimit = Error.stackTraceLimit ? Error.stackTraceLimit : 9;
        var max = Math.min(stackLimit, st.length);
        for (var i = 0; i < max; i++) {
          var arr = st[i].split(':');
          var msg = "    at " + st[i];
          if (arr.length == 3) {
            err.stack[i] = new _stackProto(msg, arr[0], arr[1], arr[2]);
          } else {
            err.stack[i] = new _stackProto(msg, err.fileName, err.lineNumber,
                    err.columnNumber);
          }
        }

        err.stack.toString = function() {
          var arr = err.stack.join("\n");
          if (arr.length > 4 && arr.substr(arr.length - 3, 2) == 'at')
            arr = arr.substr(0, arr.length - 3);
          if (!err.name) err.name = "Error";
          return err.name + ": " + err.message + "\n" + arr;
        }
      }
    };

    if (!global.gc) {
      global.gc = function () {
        jxcore.tasks.forceGC();
      }
    }
  }

  var hasRestartListener = false;

  function startup() {
    var EventEmitter = NativeModule.require('events').EventEmitter;

    process.__proto__ = Object.create(EventEmitter.prototype, {
      constructor: {
        value: process.constructor
      }
    });
    EventEmitter.call(process);

    process.EventEmitter = EventEmitter; // process.EventEmitter is deprecated

    // do this good and early, since it handles errors.
    startup.processFatal();

    startup.globalVariables();
    startup.globalTimeouts();
    startup.globalConsole();
    startup.globalJXcore();

    startup.processAssert();
    startup.processConfig();
    startup.processNextTick();
    startup.processStdio();
    startup.processKillAndExit();
    startup.processSignalHandlers();

    startup.processChannel();

    startup.resolveArgv0();

    if (!process.subThread) {
      var __exitCode = 0;
      var __pstart = Date.now();
      var __kill = function (code) {
        try {// previously skipped (process.FatalException)
          process.emit('exit', code);
        } catch (er) {
          // nothing to be done about it at this point.
        }
        if (!code && code !== 0)
          process.exit(__exitCode);
        else
          process.exit(code);
      };
      var resetBody = function (code) {
        if (Date.now() - __pstart < 5000) {
          var diff = Date.now() - __pstart;
          console
            .error("Automatic reset feature is only applicable to applications active for at least 5000 milliseconds");
          console.error("The application was alive for ", diff, "ms");
          __kill(code);
          return;
        }

        var fs = NativeModule.require('fs');
        var path = NativeModule.require('path');
        var fname = "./jxcore." + path.basename(process.mainModule.filename)
          + ".log";

        if (fs.existsSync(fname)) fs.unlinkSync(fname);

        var spawn = NativeModule.require('child_process').spawn, out = fs
          .openSync(fname, 'a'), err = fs.openSync(fname, 'a');

        var cmd = process.argv[0];
        var params = process.argv.slice(1);
        params.push("$JX$CORE_APP_RESET");

        var child = spawn(cmd, params, {
          detached: true,
          stdio: ['ignore', out, err]
        });

        child.unref();
        __kill(code);
      };

      process.on('$$restart', function (code) {
        try {
          __exitCode = code;
          if (startup.hasResetCB())
            process.emit('restart', resetBody, code);
          else {
            process.exit(code);
          }
        } catch (e) {
          console.error("process.on -> restart", e);
        }
      });
    }

    var co = NativeModule.require('console');
    if (process.subThread) {
      var arr = new Array();
      for (var o in process.argv) {
        if ((process.argv[o].indexOf("--") !== 0)) {
          arr.push(process.argv[o]);
        }
      }
      process.argv = arr;
    }

    var process_restarted = false;
    if (process.argv[process.argv.length - 1] == "$JX$CORE_APP_RESET") {
      process_restarted = true;
      process.argv = process.argv.slice(0, process.argv.length - 1);
    }

    // There are various modes that Node can run in. The most common two
    // are running from a script and running the REPL - but there are a few
    // others like the debugger or running --eval arguments. Here we decide
    // which mode we run in.

    var getActiveFolder = function () {
      var sep;
      try {
        sep = process.cwd() + NativeModule.require('path').sep;
      } catch (e) {
        co.error("Perhaps the path you are running on is not "
        + "exist any more. Please revisit the path and try again.");
        process.exit(1);
      }
      return sep;
    };

    var isPackaged = checkSource(process.subThread);
    if (process.isPackaged != isPackaged) {
      if (process.subThread) {
        process.isPackaged = isPackaged;
      } else {
        co.error("Binary corrupted");
        process.exit(1);
      }
    }

    if (!process.isPackaged) {
      delete (process.isPackaged);
    } else {
      process._EmbeddedSource = true;
    }

    var __ops = {
      install: false,
      compile: false,
      package: false,
      packagetojx: false
    };

    if (!process._EmbeddedSource && !process.subThread) {
      for (var o in __ops) {
        if (process.argv[1] === o) {
          var sep = getActiveFolder();
          __ops[o] = !NativeModule.require('fs').existsSync(sep + o + ".js");
          if (!__ops[o]) {
            co.error("while [jx " + o
            + "] is a special command, the current folder has " + o
            + ".js, running that instead.\n");
          }
          break;
        }
      }
    }

    if (__ops.compile) {
      if (!process.argv[2]) {
        jxcore.utils.console.log('JXcore Packaging System');
        co.log('please provide the jxp file to compile');
        co.log('usage: compile [jxp file to compile]');
        co.log('');
        return;
      }
      co.log('Processing the project file..');
      _jx(process.argv, __ops);
      return;
    } else if (__ops.package || __ops.packagetojx) {
      if (process.argv.length < 3) {
        jxcore.utils.console.log('J' + 'Xcore Packaging System');
        co.log("usage: package [main javascript file] [project name]");
        co.log('');
        process.exit();
        return;
      }
      co.log('Processing the folder..');
      cjx(process.argv);
      return;
    } else if (__ops.install) {
      NativeModule.installer();
      return;
    } else if (process.argv[1] == 'debug') {
      // Start the debugger agent
      var d = NativeModule.require('_debugger');
      d.start();
    } else if (process._eval != null) {
      // User passed '-e' or '--eval' arguments to Node.
      NativeModule.require('_jx_config');
      evalScript('[eval]');
    } else if (process.argv[1] || process._EmbeddedSource) {
      if (process.subThread && process._EmbeddedSource) {
        process._EmbeddedSource = false;
      }

      var __debug = false;
      if (!process._EmbeddedSource) {
        __debug = global.v8debug && process.execArgv.some(function (arg) {
          return arg.match(/^--debug-brk(=[0-9]*)?$/);
        });

        var mterCount = 0;
        var mter = process.argv.length > 1
          && (process.argv[1] == 'mt' || process.argv[1] == 'mt-keep');

        if (!mter) {
          if (process.argv.length < 2)
            mterCount = -1;
          else {
            mter = process.argv[1].indexOf('mt:') === 0
            || process.argv[1].indexOf('mt-keep:') === 0;
            if (mter) {
              var number = process.argv[1].split(':')[1];
              try {
                mterCount = parseInt(number);
              } catch (e) {
                mterCount = -1;
              }
            }
          }
        } else {
          mterCount = 2;
        }

        var fo;
        if (mter) {
          if (mterCount >= 2 && mterCount <= 16)
            process._MTED = true;
          else {
            co.log('\nusage: mt/mt-keep [source file]\n');
            co.log('remarks: by default it starts with 2 threads.');
            co.log('mt:n OR mt-keep:n (n) the number of threads. (max 16)');
            co.log('jx mt:3 [source file]\n');
            process.exit();
            return;
          }
        } else if (!process.subThread
          && process.argv[1].toLowerCase() == 'monitor') {
          process._Monitor = true;
        }

        if (!process.subThread && !process._Monitor) {
          var path = NativeModule.require('path');
          if (process._MTED) {
            process.argv[2] = path.resolve(process.argv[2]);
          } else {
            process.argv[1] = path.resolve(process.argv[1]);
          }

          NativeModule.require('_jx_config');

          // If this is a worker in cluster mode, start up the communication
          // channel.
          if (process.env.NODE_UNIQUE_ID) {
            var cluster = NativeModule.require('cluster');

            cluster._setupWorker();

            // Make sure it's not accidentally inherited by child processes.
            delete process.env.NODE_UNIQUE_ID;
          }
        }
      }

      var Module = NativeModule.require('module');

      if (__debug) {
        var debugTimeout = +process.env.NODE_DEBUG_TIMEOUT || 50;
        setTimeout(Module.runMain, debugTimeout);
      } else {
        if (process_restarted) {
          // in case the main app was waiting for threads (~500ms) node.cc
          var restartInterval = +process.env.JX_RESTART_INTERVAL || 500;
          setTimeout(Module.runMain, restartInterval);
        } else {
          Module.runMain();
        }
      }
    } else {
      var Module = NativeModule.require('module');

      // If -i or --interactive were passed, or stdin is a TTY.
      if (process._forceRepl || NativeModule.require('tty').isatty(0)) {
        // REPL
        var opts = {
          useGlobal: true,
          ignoreUndefined: false
        };
        if (parseInt(process.env['NODE_NO_READLINE'], 10)) {
          opts.terminal = false;
        }
        if (parseInt(process.env['NODE_DISABLE_COLORS'], 10)) {
          opts.useColors = false;
        }
        var repl = Module.requireRepl().start(opts);
        repl.on('exit', function () {
          process.exit();
        });

      } else {
        // Read all of stdin - execute it.
        process.stdin.setEncoding('utf8');

        var code = '';
        process.stdin.on('data', function (d) {
          code += d;
        });

        process.stdin.on('end', function () {
          process._eval = code;
          evalScript('[stdin]');
        });
      }
    }
  }

  startup.globalVariables = function () {
    global.process = process;
    global.global = global;
    global.GLOBAL = global;
    global.root = global;
    global.Buffer = NativeModule.require('buffer').Buffer;
    process.binding('buffer').setFastBufferConstructor(global.Buffer);
    process.domain = null;
    process._exiting = false;
    var tw = process.binding("thread_wrap");

    process.sendToMain = function (obj) {
      if (process.__reset) return;

      if (!process.subThread)
        jxcore.tasks.emit('message', -1, obj);
      else
        tw.sendToAll(-1, JSON.stringify({
          threadId: process.threadId,
          params: obj
        }), process.threadId);
    };

    process.sendToThread = function (threadId, obj) {
      if (process.__reset) return;
      if (threadId == null || threadId == undefined) {
        throw new TypeError(
          "threadId must be defined");
      }

      if (threadId < -1 || threadId > 63) {
        throw new RangeError(
          "threadId must be between -1 and 63");
      }
      tw.sendToAll(threadId, JSON.stringify({
        tid: process.threadId,
        data: obj
      }), process.threadId);
    };

    process.sendToThreads = function (obj) {
      if (process.__reset) return;

      tw.sendToAll(-2, JSON.stringify({
        tid: process.threadId,
        data: obj
      }), process.threadId);
    };

    process.keepAlive = function () {
      // DUMMY
      console.warn("process.keepAlive shouldn't be called from main thread");
    };

    process.release = function () {
      // DUMMY
      console.warn("process.release shouldn't be called from main thread");
    };

    process.unloadThread = function () {
      // DUMMY
      console.warn("process.unloadThread shouldn't be called from main thread");
    };
  };

  startup.globalTimeouts = function () {
    global.setTimeout = function () {
      var t = NativeModule.require('timers');
      return t.setTimeout.apply(this, arguments);
    };

    global.setInterval = function () {
      var t = NativeModule.require('timers');
      return t.setInterval.apply(this, arguments);
    };

    global.clearTimeout = function () {
      var t = NativeModule.require('timers');
      return t.clearTimeout.apply(this, arguments);
    };

    global.clearInterval = function () {
      var t = NativeModule.require('timers');
      return t.clearInterval.apply(this, arguments);
    };

    global.setImmediate = function () {
      var t = NativeModule.require('timers');
      return t.setImmediate.apply(this, arguments);
    };

    global.clearImmediate = function () {
      var t = NativeModule.require('timers');
      return t.clearImmediate.apply(this, arguments);
    };
  };

  startup.globalConsole = function () {
    global.__defineGetter__('console', function () {
      return NativeModule.require('console');
    });
    Object.defineProperty(global, '__callstack', {
      get: function () {
        var orig = Error.prepareStackTrace;
        Error.prepareStackTrace = function (_, stack) {
          return stack;
        };
        var err = new Error;
        Error.captureStackTrace(err, arguments.callee);
        var stack = err.stack;
        Error.prepareStackTrace = orig;
        return stack;
      }
    });
  };

  startup.globalJXcore = function () {
    global.__defineGetter__('jxcore', function () {
      var obj = {};
      obj.__defineGetter__('utils', function () {
        return NativeModule.require('_jx_utils');
      });
      obj.__defineGetter__('store', function () {
        return NativeModule.require('_jx_memStore').store;
      });
      obj.__defineGetter__('tasks', function () {
        return NativeModule.require('_jx_tasks');
      });
      obj.__defineGetter__('monitor', function () {
        return NativeModule.require('_jx_monitor');
      });
      return obj;
    });

    Function.prototype.runTask = function (params, callback, objects) {
      jxcore.tasks.addTask(this, params, callback, objects);
    };

    Function.prototype.runOnce = function (params) {
      jxcore.tasks.runOnce(this, params);
    };

    Function.prototype._runTask = function (params, callback, objects) {
      jxcore.tasks._addTask(this, params, callback, objects);
    };

    Function.prototype._runOnce = function (params) {
      jxcore.tasks._runOnce(this, params);
    };
  };

  startup._lazyConstants = null;

  startup.lazyConstants = function () {
    if (!startup._lazyConstants) {
      startup._lazyConstants = process.binding('constants');
    }
    return startup._lazyConstants;
  };

  startup.processFatal = function () {
    // call into the active domain, or emit uncaughtException,
    // and exit if there are no listeners.
    process._fatalException = function (er) {
      var caught = false;
      if (process.domain) {
        var domain = process.domain;
        var domainModule = NativeModule.require('domain');
        var domainStack = domainModule._stack;

        // ignore errors on disposed domains.
        //
        // XXX This is a bit stupid. We should probably get rid of
        // domain.dispose() altogether. It's almost always a terrible
        // idea. --isaacs
        if (domain._disposed) return true;

        er.domain = domain;
        er.domainThrown = true;
        // wrap this in a try/catch so we don't get infinite throwing
        try {
          // One of three things will happen here.
          //
          // 1. There is a handler, caught = true
          // 2. There is no handler, caught = false
          // 3. It throws, caught = false
          //
          // If caught is false after this, then there's no need to exit()
          // the domain, because we're going to crash the process anyway.
          caught = domain.emit('error', er);

          // Exit all domains on the stack. Uncaught exceptions end the
          // current tick and no domains should be left on the stack
          // between ticks.
          var domainModule = NativeModule.require('domain');
          domainStack.length = 0;
          domainModule.active = process.domain = null;
        } catch (er2) {
          // The domain error handler threw! oh no!
          // See if another domain can catch THIS error,
          // or else crash on the original one.
          // If the user already exited it, then don't double-exit.
          if (domain === domainModule.active) domainStack.pop();
          if (domainStack.length) {
            var parentDomain = domainStack[domainStack.length - 1];
            process.domain = domainModule.active = parentDomain;
            caught = process._fatalException(er2);
          } else
            caught = false;
        }
      } else {
        caught = process.emit('uncaughtException', er);
      }
      // if someone handled it, then great. otherwise, die in C++ land
      // since that means that we'll exit the process, emit the 'exit' event
      if (!caught) {
        try {
          if (!process._exiting && !hasRestartListener) {
            process._exiting = true;
            process.emit('exit', 1);
          }
        } catch (er) {
          // nothing to be done about it at this point.
        }
      }
      // if we handled an error, then make sure any ticks get processed
      if (caught) {
        process._needTickCallback();
      }
      return caught;
    };
  };

  var assert;
  startup.processAssert = function () {
    // Note that calls to assert() are pre-processed out by JS2C for the
    // normal build of node. They persist only in the node_g build.
    // Similarly for debug().
    assert = process.assert = function (x, msg) {
      if (!x) throw new Error(msg || 'assertion error');
    };
  };

  startup.processConfig = function () {
    // used for `process.config`, but not a real module
    var config = NativeModule._source.config;
    delete NativeModule._source.config;

    // strip the gyp comment line at the beginning
    config = config.split('\n').slice(1).join('\n').replace(/'/g, '"');

    process.config = JSON.parse(config, function (key, value) {
      if (value === 'true') return true;
      if (value === 'false') return false;
      return value;
    });
  };

  // process.binding is wrapped in order to keep JXP specific updates from the
  // output
  process.binding = function (name, skip) {
    if (!skip) {
      skip = 99;
    }

    var res = process.jx_binding(name, skip);
    if (res && name == 'natives') {
      var ind = res['module'].indexOf('/**JXCORE_JXP**/');
      if (ind >= 0) {
        res['module'] = res['module'].substr(0, ind);
      }

      ind = res['fs'].indexOf('/**JXCORE_JXP**/');
      if (ind >= 0) {
        res['fs'] = res['fs'].substr(0, ind);
      }
    }
    return res;
  };

  process.dlopen = function (module, filename) {
    var isWindows = process.platform === 'win32';

    if (isWindows) {
      filename = filename.toLowerCase();
    }

    var gpath = process._dlopen(module, filename);
    if (gpath) {
      var fs = NativeModule.require('fs');
      var pathModule = NativeModule.require('path');
      var err = new Error(
        "On this system, processes are limited to run native modules only from global repository. "
        + filename
        + " wasn't exist. Please make sure it's a standard JXcore / node.js native module.");
      var i = filename.lastIndexOf("node_modules");
      if (i < 0) {
        throw err;
      }
      var mod_str = filename.substr(i);

      if (fs.existsSync(gpath + mod_str)) {
        process._dlopen(module, gpath + mod_str);
      } else {
        throw err;
      }
    }
  };

  startup.processNextTick = function () {
    var _needTickCallback = process._needTickCallback;
    var nextTickQueue = new Array();
    var needSpinner = true;
    var inTick = false;

    // this infobox thing is used so that the C++ code in src/node.cc
    // can have easy accesss to our nextTick state, and avoid unnecessary
    // calls into process._tickCallback.
    // order is [length, index, depth]
    // Never write code like this without very good reason!
    var infoBox = process._tickInfoBox;
    var length = 0;
    var index = 1;
    var depth = 2;

    // needs to be accessible from cc land
    process._currentTickHandler = _nextTick;
    process._nextDomainTick = _nextDomainTick;
    process._tickCallback = _tickCallback;
    process._tickDomainCallback = _tickDomainCallback;
    process._tickFromSpinner = _tickFromSpinner;

    process.nextTick = function nextTick(cb) {
      process._currentTickHandler(cb);
    };

    // the maximum number of times it'll process something like
    // nextTick(function f(){nextTick(f)})
    // It's unlikely, but not illegal, to hit this limit. When
    // that happens, it yields to libuv's tick spinner.
    // This is a loop counter, not a stack depth, so we aren't using
    // up lots of memory here. I/O can sneak in before nextTick if this
    // limit is hit, which is not ideal, but not terrible.

    process.maxTickDepth = 1000;

    function tickDone(tickDepth_) {
      if (infoBox[length] !== 0) {
        if (infoBox[length] <= infoBox[index]) {
          nextTickQueue = new Array();
          infoBox[length] = 0;
        } else {
          nextTickQueue.splice(0, infoBox[index]);
          infoBox[length] = nextTickQueue.length;
          if (needSpinner) {
            _needTickCallback();
            needSpinner = false;
          }
        }
      }
      inTick = false;
      infoBox[index] = 0;
      infoBox[depth] = tickDepth_;
    }

    var last_time_maxTickWarn = 0;

    function maxTickWarn() {
      if (process.versions.sm) {
        // (TODO) (obastemur) ..ugly hack.
        var now = Date.now();
        if (now - last_time_maxTickWarn < 2) return;
        last_time_maxTickWarn = now;
      }

      var msg = '(node) warning: Recursive process.nextTick detected. '
        + 'This will break in the next version of node. '
        + 'Please use setImmediate for recursive deferral.';

      if (process.throwDeprecation)
        throw new Error(msg);
      else if (process.traceDeprecation)
        console.trace(msg);
      else {
        console.error(msg);
      }
    }

    function _tickFromSpinner() {
      needSpinner = true;
      // coming from spinner, reset!
      if (infoBox[depth] !== 0) infoBox[depth] = 0;
      // no callbacks to run
      if (infoBox[length] === 0) return infoBox[index] = infoBox[depth] = 0;
      process._tickCallback();
    }

    // run callbacks that have no domain
    // using domains will cause this to be overridden
    function _tickCallback() {
      var callback, nextTickLength, threw;

      if (inTick) return;
      if (infoBox[length] === 0) {
        infoBox[index] = 0;
        infoBox[depth] = 0;
        return;
      }
      inTick = true;

      while (infoBox[depth]++ < process.maxTickDepth) {
        nextTickLength = infoBox[length];
        if (infoBox[index] === nextTickLength) return tickDone(0);

        while (infoBox[index] < nextTickLength) {
          callback = nextTickQueue[infoBox[index]++].callback;
          // threw = true;
          try {
            callback();
            // threw = false;
          }
            // JIT doesn't support try/finally!
            // finally {
            // if (threw)
            // tickDone(infoBox[depth]);
            // }
          catch (ee) {
            tickDone(infoBox[depth]);
            throw ee;
          }
        }
      }

      tickDone(0);
    }

    function _tickDomainCallback() {
      var nextTickLength, tock, callback, threw;

      // if you add a nextTick in a domain's error handler, then
      // it's possible to cycle indefinitely. Normally, the tickDone
      // in the finally{} block below will prevent this, however if
      // that error handler ALSO triggers multiple MakeCallbacks, then
      // it'll try to keep clearing the queue, since the finally block
      // fires *before* the error hits the top level and is handled.
      if (infoBox[depth] >= process.maxTickDepth) {
        return _needTickCallback();
      }

      if (inTick) return;
      inTick = true;

      // always do this at least once. otherwise if process.maxTickDepth
      // is set to some negative value, or if there were repeated errors
      // preventing depth from being cleared, we'd never process any
      // of them.
      while (infoBox[depth]++ < process.maxTickDepth) {
        nextTickLength = infoBox[length];
        if (infoBox[index] === nextTickLength) return tickDone(0);

        while (infoBox[index] < nextTickLength) {
          tock = nextTickQueue[infoBox[index]++];
          callback = tock.callback;
          if (tock.domain) {
            if (tock.domain._disposed) continue;
            tock.domain.enter();
          }
          threw = true;
          try {
            callback();
            threw = false;
          } finally {
            // finally blocks fire before the error hits the top level,
            // so we can't clear the depth at this point.
            if (threw) tickDone(infoBox[depth]);
          }
          if (tock.domain) {
            tock.domain.exit();
          }
        }
      }

      tickDone(0);
    }

    function _nextTick(callback) {
      // on the way out, don't bother. it won't get fired anyway.
      if (process._exiting) return;
      if (infoBox[depth] >= process.maxTickDepth) {
        maxTickWarn();
      }

      var obj = {
        callback: callback,
        domain: null
      };

      nextTickQueue.push(obj);
      infoBox[length]++;

      if (needSpinner) {
        _needTickCallback();
        needSpinner = false;
      }
    }

    function _nextDomainTick(callback) {
      // on the way out, don't bother. it won't get fired anyway.
      if (process._exiting) return;

      if (infoBox[depth] >= process.maxTickDepth) maxTickWarn();

      var obj = {
        callback: callback,
        domain: process.domain
      };

      nextTickQueue.push(obj);
      infoBox[length]++;

      if (needSpinner) {
        _needTickCallback();
        needSpinner = false;
      }
    }
  };

  function evalScript(name) {
    var Module = NativeModule.require('module');
    var path = NativeModule.require('path');
    var cwd = process.cwd();

    var module = new Module(name);
    module.filename = path.join(cwd, name);
    module.paths = Module._nodeModulePaths(cwd);
    var script = process._eval;
    if (!Module._contextLoad) {
      var body = script;
      script = 'global.__filename = ' + JSON.stringify(name) + ';\n'
      + 'global.exports = exports;\n' + 'global.module = module;\n'
      + 'global.__dirname = __dirname;\n'
      + 'global.require = require;\n'
      + 'return require("vm").runInThisContext(' + JSON.stringify(body)
      + ', ' + JSON.stringify(name) + ', true);\n';
    }

    var result = module._compile(script, name + '-wrapper');
    if (process._print_eval) console.log(result);
  }

  function errnoException(errorno, syscall) {
    // TODO make this more compatible with ErrnoException from src/node.cc
    // Once all of Node is using this function the ErrnoException from
    // src/node.cc should be removed.
    var e = new Error(syscall + ' ' + errorno);
    e.errno = e.code = errorno;
    e.syscall = syscall;
    return e;
  }

  function createWritableStdioStream(fd) {
    var stream;
    var tty_wrap = process.binding('tty_wrap');

    // Note stream._type is used for test-module-load-list.js

    switch (tty_wrap.guessHandleType(fd)) {
      case 'TTY':
        var tty = NativeModule.require('tty');
        stream = new tty.WriteStream(fd);
        stream._type = 'tty';

        // Hack to have stream not keep the event loop alive.
        // See https://github.com/joyent/node/issues/1726
        if (stream._handle && stream._handle.unref) {
          stream._handle.unref();
        }
        break;

      case 'FILE':
        var fs = NativeModule.require('fs');
        stream = new fs.SyncWriteStream(fd, {
          autoClose: false
        });
        stream._type = 'fs';
        break;

      case 'PIPE':
      case 'TCP':
        var net = NativeModule.require('net');
        stream = new net.Socket({
          fd: fd,
          readable: false,
          writable: true
        });

        // FIXME Should probably have an option in net.Socket to create a
        // stream from an existing fd which is writable only. But for now
        // we'll just add this hack and set the `readable` member to false.
        // Test: ./node test/fixtures/echo.js < /etc/passwd
        stream.readable = false;
        stream.read = null;
        stream._type = 'pipe';

        // FIXME Hack to have stream not keep the event loop alive.
        // See https://github.com/joyent/node/issues/1726
        if (stream._handle && stream._handle.unref) {
          stream._handle.unref();
        }
        break;

      default:
        // Probably an error on in uv_guess_handle()
        throw new Error('Implement me. Unknown stream file type!');
    }

    // For supporting legacy API we put the FD here.
    stream.fd = fd;

    stream._isStdio = true;

    return stream;
  }

  startup.processStdio = function () {
    var stdin, stdout, stderr;

    process.__defineGetter__('stdout', function () {
      if (stdout) return stdout;
      stdout = createWritableStdioStream(1);
      stdout.destroy = stdout.destroySoon = function (er) {
        er = er || new Error('process.stdout cannot be closed.');
        stdout.emit('error', er);
      };
      if (stdout.isTTY) {
        process.on('SIGWINCH', function () {
          stdout._refreshSize();
        });
      }
      return stdout;
    });

    process.__defineGetter__('stderr', function () {
      if (stderr) return stderr;
      stderr = createWritableStdioStream(2);
      stderr.destroy = stderr.destroySoon = function (er) {
        er = er || new Error('process.stderr cannot be closed.');
        stderr.emit('error', er);
      };
      return stderr;
    });

    process.__defineGetter__('stdin', function () {
      if (stdin) return stdin;

      var tty_wrap = process.binding('tty_wrap');
      var fd = 0;

      switch (tty_wrap.guessHandleType(fd)) {
        case 'TTY':
          var tty = NativeModule.require('tty');
          stdin = new tty.ReadStream(fd, {
            highWaterMark: 0,
            readable: true,
            writable: false
          });
          break;

        case 'FILE':
          var fs = NativeModule.require('fs');
          stdin = new fs.ReadStream(null, {
            fd: fd,
            autoClose: false
          });
          break;

        case 'PIPE':
        case 'TCP':
          var net = NativeModule.require('net');
          stdin = new net.Socket({
            fd: fd,
            readable: true,
            writable: false
          });
          break;

        default:
          // Probably an error on in uv_guess_handle()
          throw new Error('Implement me. Unknown stdin file type!');
      }

      // For supporting legacy API we put the FD here.
      stdin.fd = fd;

      // stdin starts out life in a paused state, but node doesn't
      // know yet. Explicitly to readStop() it to put it in the
      // not-reading state.
      if (stdin._handle && stdin._handle.readStop) {
        stdin._handle.reading = false;
        stdin._readableState.reading = false;
        stdin._handle.readStop();
      }

      // if the user calls stdin.pause(), then we need to stop reading
      // immediately, so that the process can close down.
      stdin.on('pause', function () {
        if (!stdin._handle) return;
        stdin._readableState.reading = false;
        stdin._handle.reading = false;
        stdin._handle.readStop();
      });

      return stdin;
    });

    process.openStdin = function () {
      process.stdin.resume();
      return process.stdin;
    };
  };

  startup.processKillAndExit = function () {
    process.exit = function (code) {
      if (!process._exiting) {
        process._exiting = true;
        process.emit('exit', code || 0);
      }
      process.reallyExit(code || 0);
    };

    process.kill = function (pid, sig) {
      var r;

      // preserve null signal
      if (0 === sig) {
        r = process._kill(pid, 0);
      } else {
        sig = sig || 'SIGTERM';
        if (startup.lazyConstants()[sig] && sig.slice(0, 3) === 'SIG') {
          r = process._kill(pid, startup.lazyConstants()[sig]);
        } else {
          throw new Error('Unknown signal: ' + sig);
        }
      }

      if (r) {
        throw errnoException(process._errno, 'kill');
      }

      return true;
    };
  };

  startup.processSignalHandlers = function () {
    // Load events module in order to access prototype elements on process like
    // process.addListener.
    var signalWraps = {};
    var addListener = process.addListener;
    var removeListener = process.removeListener;

    function isSignal(event) {
      return event.slice(0, 3) === 'SIG'
        && startup.lazyConstants().hasOwnProperty(event);
    }

    startup.hasResetCB = function () {
      return hasRestartListener;
    };

    // Wrap addListener for the special signal types
    process.on = process.addListener = function (type, listener) {
      if (type == 'restart') {
        hasRestartListener = true;
      }

      if (isSignal(type) && !signalWraps.hasOwnProperty(type)) {
        var Signal = process.binding('signal_wrap').Signal;
        var wrap = new Signal();

        wrap.unref();

        wrap.onsignal = function () {
          process.emit(type);
        };

        var signum = startup.lazyConstants()[type];
        var r = wrap.start(signum);
        if (r) {
          wrap.close();
          throw errnoException(process._errno, 'uv_signal_start');
        }

        signalWraps[type] = wrap;
      }

      return addListener.apply(this, arguments);
    };

    process.removeListener = function (type, listener) {
      var ret = removeListener.apply(this, arguments);
      if (isSignal(type)) {
        assert(signalWraps.hasOwnProperty(type));

        if (this.listeners(type).length === 0) {
          signalWraps[type].close();
          delete signalWraps[type];
        }
      }

      return ret;
    };
  };

  startup.processChannel = function () {
    // If we were spawned with env NODE_CHANNEL_FD then load that up and
    // start parsing data from that stream.
    if (process.env.NODE_CHANNEL_FD) {
      var fd = parseInt(process.env.NODE_CHANNEL_FD, 10);
      assert(fd >= 0);

      // Make sure it's not accidentally inherited by child processes.
      delete process.env.NODE_CHANNEL_FD;

      var cp = NativeModule.require('child_process');

      cp._forkChild(fd);
      assert(process.send);
    }
  }

  startup.resolveArgv0 = function () {
    var cwd;
    try {
      cwd = process.cwd();
    } catch (e) {
      console
        .error("Error: You may not have a read access on current folder or a file system link to current folder removed. Please revisit the folder and make sure you have an access.");
      process.exit(1);
    }
    var isWindows = process.platform === 'win32';

    // Make process.argv[0] into a full path, but only touch argv[0] if it's
    // not a system $PATH lookup.
    // TODO: Make this work on Windows as well. Note that "node" might
    // execute cwd\node.exe, or some %PATH%\node.exe on Windows,
    // and that every directory has its own cwd, so d:node.exe is valid.
    var argv0 = process.argv[0];
    if (!isWindows && argv0.indexOf('/') !== -1 && argv0.charAt(0) !== '/') {
      var path = NativeModule.require('path');
      process.argv[0] = path.join(cwd, process.argv[0]);
    }
  };

  // Below you find a minimal module system, which is used to load the node
  // core modules found in lib/*.js. All core modules are compiled into the
  // node binary, so they can be loaded faster.

  var Script = process.binding('evals').NodeScript;
  var runInThisContext = Script.runInThisContext;

  function NativeModule(id) {
    this.filename = id + '.js';
    this.id = id;
    this.exports = {};
    this.loaded = false;
  }

  NativeModule.Roots = {};
  NativeModule.RootsLength = 0;
  process.binding('natives', 1);
  NativeModule._cache = {};

  NativeModule.require = function (id) {
    if (!id) {
      throw new TypeError(
        "NativeModule.require expects name of the module");
    }

    if (id == 'native_module') {
      return NativeModule;
    }

    var cached = NativeModule.getCached(id);
    if (cached) {
      return cached.exports;
    }

    if (!NativeModule.exists(id)) {
      throw new Error('No such native module '
      + id);
    }

    if (id.indexOf("_jx_") < 0) {
      process.moduleLoadList.push('NativeModule ' + id);
    }

    var nativeModule = new NativeModule(id);

    nativeModule.cache();
    nativeModule.compile();

    return nativeModule.exports;
  };

  NativeModule.getCached = function (id) {
    if (NativeModule._cache.hasOwnProperty(id)) {
      return NativeModule._cache[id];
    } else {
      return null;
    }
  };

  NativeModule.exists = function (id) {
    if (id == 'config') return false;
    return NativeModule.hasOwnProperty(id);
  };

  NativeModule.wrap = function (script) {
    return NativeModule.wrapper[0] + script + NativeModule.wrapper[1];
  };

  NativeModule.wrapper = [
    '(function (exports, require, module, __filename, __dirname, setTimeout, setInterval, process) { ',
    '\n});'];

  NativeModule.prototype.compile = function () {
    var source = NativeModule.getSource(this.id);
    source = NativeModule.wrap(source, this.id === 'module');

    var fn = runInThisContext(source, this.filename, true, 0);

    fn(this.exports, NativeModule.require, this, this.filename, undefined,
      global.setTimeout, global.setInterval, global.process);

    this.loaded = true;
  };

  NativeModule.prototype.cache = function () {
    NativeModule._cache[this.id] = this;
  };

  function stripBOM(content) {
    if (content.charCodeAt(0) === 0xFEFF) {
      content = content.slice(1);
    }
    return content;
  }

  var getOptions = function (name, defaultValue) {
    for (var o = 0; o < process.argv.length; o++) {
      if (process.argv[o].toLowerCase() == name.toLowerCase()) {
        if (process.argv.length > o + 1 && typeof process.argv[o + 1] === "string" && process.argv[o + 1].slice(0,1) !== "-")
          return process.argv[o + 1];
        else
          return (typeof defaultValue === "undefined" ? true : defaultValue);
      }
    }
    return (typeof defaultValue === "undefined" ? false : defaultValue);
  };

  var getBoolOption = function(name, defaultValue) {
    var _val = getOptions(name, "not_found");
    if (_val === true || _val === false) return _val;
    if (_val === "not_found" && process.argv.indexOf(name) === -1)
      return defaultValue;
    var _str = _val.toString().toLowerCase();
    return (_str === "no" || _str === "false" || _str === "0") ? false : true;
  };

  var getArrayOption = function(name, defaultValue) {
    var _val = getOptions(name, defaultValue);
    var arr = null;
    try {
      arr = _val.split(",");
      if (!arr.length) arr = null;
    } catch (ex) { }

    if (!arr)
      return null

    for(var o in arr)
      arr[o] = arr[o].trim();

    return arr;
  };

  var parseValues = function(name) {
    var parms = getOptions(name);

    if (!parms)
      return null;

    if (typeof parms !== "string")
      return { isWildcardMatching : function() {} };

    var path = NativeModule.require('path');
    var fs = NativeModule.require('fs');
    var ret = { regexes : [] };

    var specials = [ "\\", "^", "$", ".", "|", "+", "(", ")", "[", "]", "{", "}" ];  // without '*' and '?'

    parms = parms.split(',');
    for (var o in parms) {

      if (parms[o].indexOf("*") !== -1 || parms[o].indexOf("?") !== -1) {

        var r = parms[o];
        for (var i in specials)
          r = r.replace(new RegExp("\\" + specials[i], "g"), "\\" + specials[i]);

        r = r.replace(/\*/g, '.*').replace(/\?/g, '.{1,1}');
        ret.regexes.push(new RegExp('^' + r + '$'));
        ret.regexes.push(new RegExp('^' + path.join(process.cwd(), r) + '$'));
        continue;
      }

      ret[parms[o]] = 1;
      var _path;
      if (parms[o].indexOf(process.cwd()) === -1) {
        // relative path was given
        _path = path.join(process.cwd(), parms[o]);
      } else {
        // absolute path was given
        _path = parms[o];
      }

      if (_path.slice(-1) === path.sep)
        _path = _path.slice(0, _path.length - 1);
      if (fs.existsSync(_path))
        ret[_path] = 2;
    }

    ret.isWildcardMatching = function(file, file_path) {
      for(var o in ret.regexes) {
        var regex = ret.regexes[o];
        if (regex.test(file) || regex.test(file_path))
          return true;
      }
    };

    return ret;
  };

  var checkOff, add;

  var getFiles = function (folder, startup_path) {
    var fz = {
      f: [],
      a: []
    };
    var path = NativeModule.require('path');
    var fs = NativeModule.require('fs');

    var mainPath = process.cwd();
    var onFirst = false;
    if (!folder) {
      onFirst = true;
      folder = process.cwd() + path.sep;
    }

    // if startup_path is relative:
    if (startup_path && startup_path.indexOf(mainPath) === -1)
      startup_path = path.join(mainPath, startup_path);

    if (checkOff === undefined)
      checkOff = parseValues("-slim");

    if (add === undefined)
      add = parseValues("-add");

    var files = fs.readdirSync(folder);
    for (var o in files) {
      var file = files[o];
      var file_path = path.join(folder, file);

      if (checkOff && (checkOff[file] === 1 || checkOff[file_path] === 2 || checkOff.isWildcardMatching(file, file_path)))
        continue;

      var stat = fs.statSync(file_path);

      if (add && startup_path !== file_path && stat.isFile()) {

        var tmp = file_path;
        var canAdd = false;
        do {
          var basename = path.basename(tmp);
          if (add[basename] === 1 || add[tmp] === 2 || add.isWildcardMatching(file, file_path)) {
            canAdd = true;
            break;
          }

          var dirname = path.dirname(tmp);
          // will loop only until the root dir
          if (dirname === tmp) break;
          tmp = dirname;
        }
        while (true);

        if (!canAdd)
          continue;
      }

      if (stat.isDirectory()) {
        if (file.indexOf('.') != 0) {
          var az = getFiles(file_path, startup_path);
          fz.f = fz.f.concat(az.f);
          fz.a = fz.a.concat(az.a);
        }
        continue;
      }

      var ext = path.extname(file);
      var ufile = file.toUpperCase();
      {
        if (ext == ".js" || ext == ".json") {
          fz.f.push(path.relative(mainPath, file_path));
        } else if (onFirst && (ufile == "LICENSE-MIT" || ufile == "LICENSE")) {
          fz.license = path.relative(mainPath, file_path);
          fz.a.push(path.relative(mainPath, file_path));
        } else if (onFirst && (ufile == "README" || ufile == "README.MD")) {
          fz.readme = path.relative(mainPath, file_path);
          fz.a.push(path.relative(mainPath, file_path));
        } else {
          fz.a.push(path.relative(mainPath, file_path));
        }
      }
    }
    return fz;
  };

  // returns argv[3], or if not provided: basename(argv[2)
  var getPackageName = function(argv) {
    var name = null;
    if (argv.length >= 4)
      name = argv[3];
    if (!name || name.slice(0,1) === "-") {
      name = argv[2];
      if (name.slice(-3).toLowerCase() === ".js") {
        var path = NativeModule.require('path');
        name = path.basename(name.slice(0, name.length - 3));
      }
    }
    return name;
  };

  var cjx = function (argv) {
    var path = NativeModule.require('path');
    var console = NativeModule.require('console');
    var fs = NativeModule.require('fs');

    var executer = null;
    var sss = argv[2].split('|');
    if (sss.length > 1) executer = sss[1];
    var fol = sss[0];
    fol = (path.relative(process.cwd(), fol));

    if (!fs.existsSync(path.join(process.cwd(), fol))) {
      jxcore.utils.console.error("Project startup file does not exist:", fol, "red");
      process.exit(1);
    }

    var startup_extension = path.extname(fol);
    if (startup_extension.toLowerCase() != '.js') {
      jxcore.utils.console.log("Project startup file must have a .js extension.", "red");
      process.exit(1);
    }

    var jxp = {
      "name": getPackageName(argv),
      "version": getOptions("-version", "1.0"),
      "author": getOptions("-author", ""),
      "description": getOptions("-description", ""),
      "company": getOptions("-company", ""),
      "website": getOptions("-website", ""),
      "package": null,
      "startup": fol,
      "execute": executer,
      "extract": getBoolOption("-extract", false),
      "output": null,
      "files": [],
      "assets": [],
      "preInstall": getArrayOption("-preInstall", null),
      "library": getBoolOption("-library", true),
      "license_file": null,
      "readme_file": null,
      "fs_reach_sources": getBoolOption("-fs_reach_sources", true)
    };

    if (getOptions('-native')) {
      jxp.native = true;
    }

    try {
      var fs = NativeModule.require('fs');
      var fz = getFiles(null, fol);
      jxp.files = fz.f;
      jxp.assets = fz.a;
      jxp.license_file = fz.license;
      jxp.readme_file = fz.readme;

      fin = jxp.name + ".jxp";
      jxp.output = jxp.name + ".jx";

      fs.writeFileSync(fin, JSON.stringify(jxp, null, '\t'));

      console.log("JXP project file (" + fin + ") is ready.");
      console.log("");
      console.log("preparing the JX file..");
      _jx(["", "jx", fin]);
    } catch (e) {
      console.log(e);
    }
  };

  var nameFix = function (a) {
    if (!a) return "";

    var isw = process.platform === 'win32';
    var repFrom = isw ? /[\/]/g : /[\\]/g;
    var repTo = isw ? "\\" : "/";

    return a.replace(repFrom, repTo);
  };

  var _jx = function (argv, ops) {
    var contents = {
      pack: {},
      project: {},
      docs: {}
    };

    var console = jxcore.utils.console;
    var path = NativeModule.require('path');
    var fss = NativeModule.require('fs');

    var fn = path.resolve(argv[2]);
    var ext = ".";
    if (fn.length > 4) {
      ext = fn.substr(fn.length - 4, 4);
    }

    if (ext != '.jxp') {
      console.error("unknown JX project type '" + fn + "'", "red");
      process.exit(1);
      return;
    }

    var proj = null;
    try {
      var xt = "" + fss.readFileSync(fn);
      xt = xt.trim();
      proj = JSON.parse(stripBOM(xt));
    } catch (e) {
      console.error(e);
      process.exit(1);
      return;
    }

    if (getOptions('-native')) {
      proj.native = true;
    }

    if (!proj) {
      console.error("corrupted JSON in jxp file", "red");
      process.exit(1);
      return;
    }

    if (!fss.existsSync(path.join(process.cwd(), proj.startup))) {
      console.error("Project startup file does not exist:", proj.startup, "red");
      process.exit(1);
    }

    proj.startup = "./" + proj.startup;
    var startup_extension = path.extname(proj.startup);
    if (startup_extension.toLowerCase() != '.js') {
      console.error("Project startup file must have a .js extension.", "red");
      process.exit(1);
    }

    
    if (!proj.files || !proj.files.length) {
      console
        .log("no target source file definition inside the j" + "xp",
        "red");
      process.exit(1);
      return;
    }

    if (proj.name && proj.version && proj.output) {
      var str = "Compiling " + proj.name + " " + proj.version;
      console.log(str, "green");
    } else {
      console.error(
        "'name', 'version' and 'output' fields must be defined inside the J"
        + "XP file", "red");
      process.exit(1);
      return;
    }

    contents.project = proj;

    proj = null;
    var _package = process.cwd() + path.sep + "package.json";
    var pext = fss.existsSync(_package);
    if (contents.project["package"] || pext) {
      if (!pext)
        fn = path.resolve(contents.project["package"]);
      else {
        contents.project["package"] = _package;
        fs = _package;
      }

      try {
        var x = "" + fss.readFileSync(fn);
        proj = JSON.parse(x);
      } catch (e) {
        console.error(e);
        process.exit();
        return;
      }

      if (!proj) {
        console.error("corrupted JSON in '" + fn + "' file", red);
        process.exit(1);
        return;
      }

      contents.pack = proj;
      if (!contents.project.website && (proj.website || proj.homepage)) {
        contents.project.website = proj.website ? proj.website : proj.homepage;
      }
      if (proj.bin && !contents.project.execute) {
        var set = false;
        if (proj.bin.substr) {
          contents.project.execute = proj.bin;
          set = true;
        } else if (proj.bin[0]) {
          proj.bin = proj.bin[0];
        }
        if (!set) {
          if (proj.bin.substr) {
            contents.project.execute = proj.bin;
            set = true;
          } else {
            for (var o in proj.bin) {
              if (o && proj[o]) {
                contents.project.execute = proj[o];
                set = true;
                break;
              }
            }
          }
        }

        if (set) {
          contents.project.extract = true;
        }
      }

      if (proj.version) {
        contents.project.version = proj.version;
      }

      if (proj.description) {
        contents.project.description = proj.description;
      }

      if (proj.author) {
        contents.project.author = proj.author;
      }
    }

    proj = contents.project;
    var str_dup = JSON.stringify(proj);
    var strobj = JSON.parse(str_dup);
    delete (strobj.files);
    delete (strobj.readme_file);
    delete (strobj.license_file);

    contents.PROS = "exports.$JXP=" + JSON.stringify(strobj) + ";";
    contents.stats = {};

    if (proj.fs_reach_sources && proj.fs_reach_sources !== true) {
      var arr = {};
      for (var o in proj.fs_reach_sources) {
        arr["./" + nameFix(o)] = true;
      }
      proj.fs_reach_sources = arr;
    }

    var scomp = process.binding('jxutils_wrap');
    var cw = process.stdout.columns;
    var ln_files = proj.files.length;
    for (var i = 0; i < ln_files; i++) {
      var loc = proj.files[i].trim();
      loc = nameFix(loc);
      var fn_sub = path.resolve(loc);
      console.write('adding script ', "yellow")
      console.log((loc.length < cw - 20) ? loc : "...."
      + loc.substr(loc.length - (cw - 20)));

      loc = "./" + loc;

      var content_sub = null, sub_stat = {};
      try {
        content_sub = fss.readFileSync(fn_sub);
        sub_stat = fss.statSync(fn_sub);
      } catch (e) {
        console.log("while processing ", fn_sub, 'red');
        console.log(e, 'red');
        process.exit(1);
      }

      var lo = loc.length - 5;
      if (lo < 0) lo = 0;
      var ext = path.extname(loc).toLowerCase();
      if (ext != '.json' && ext != '.js') {
        console
          .log(
          "only 'js' or 'json' files can be defined as a source code. (json and js are case sensitive)",
          "red");
        process.exit(1);
      }

      var buff;
      if (path.extname(loc).toLowerCase() != '.js')
        buff = scomp._cmp(content_sub.toString('base64')).toString('base64');
      else
        buff = scomp._cmp(stripBOM(content_sub + "")).toString('base64');

      contents.docs[loc] = buff;

      if(process.versions.sm) {
        // Stat is defined as JS_FUNCTION_TEMPLATE (node_file.h)
        // turn stat into non-callable object ECMA-5 (Object && IsCallable ->
        // stringify -> undefined)
        // https://bugzilla.mozilla.org/show_bug.cgi?id=509339
        sub_stat = Object.create(sub_stat);
      }
      
      contents.stats[loc] = JSON.stringify(sub_stat);
    }

    jxcore.tasks.forceGC();
    var warn_node = null;

    if (proj.assets) {
      var ln = proj.assets.length;
      for (var i = 0; i < ln; i++) {
        var loc = proj.assets[i].trim();
        loc = nameFix(loc);
        var fn = path.resolve(loc);
        console.write('adding asset ', "yellow")
        console.log((loc.length < cw - 19) ? loc : "...."
        + loc.substr(loc.length - (cw - 19)));

        if (path.extname(loc) == '.node') {
          warn_node = loc;
        }

        loc = "./" + loc;

        var asset_content = null;
        var _stat = {};

        try {
          asset_content = fss.readFileSync(fn);
          _stat = fss.statSync(fn);
        } catch (e) {
          console.error(e, "red");
          process.exit(1);
          return;
        }

        contents.docs[loc] = scomp._cmp(asset_content.toString('base64'))
          .toString('base64');

        if(process.versions.sm) {
          // Stat is defined as JS_FUNCTION_TEMPLATE (node_file.h)
          // turn stat into non-callable object ECMA-5 (Object && IsCallable ->
          // stringify -> undefined)
          // https://bugzilla.mozilla.org/show_bug.cgi?id=509339
          _stat = Object.create(_stat);
        }
        
        contents.stats[loc] = JSON.stringify(_stat);

        if (i % 5 == 0 || buff.length > 1e6) jxcore.tasks.forceGC();
      }
    }

    jxcore.tasks.forceGC();

    if (warn_node) {
      if (warn_node.length > 35)
        warn_node = "...." + warn_node.substr(warn_node.length - 35);
      console.log("Warning!", warn_node, "red");
      console
        .log(
        "Adding a .node (native) file into package may fail the application",
        "red");
      jxcore.utils.console.log("Check the related discussion from",
        "https://github.com/jxcore/jxcore/issues/101", "blue");
    }

    if (proj.license_file) {
      var loc = proj.license_file.trim();
      var fn = path.resolve(loc);
      console.log('adding license ' + loc);

      var ct_license = null;
      try {
        ct_license = "" + fss.readFileSync(fn);
      } catch (e) {
        console.error(e);
        process.exit();
        return;
      }

      contents.license = scomp._cmp(ct_license).toString('base64');
    }

    if (proj.readme_file) {
      var loc = proj.readme_file.trim();
      var fn = path.resolve(loc);
      console.log('adding readme ' + loc);

      var content = null;
      try {
        content = "" + fss.readFileSync(fn);
      } catch (e) {
        console.error(e);
        process.exit();
        return;
      }

      var buff = scomp._cmp(content).toString('base64');

      contents.readme = buff;
    }

    var c = JSON.stringify(contents);
    var cmped = scomp._cmp(c);

    c = null;
    jxcore.tasks.forceGC();

    var op = getPackageName(process.argv);
    if (!op || (ops && ops.compile)) {
      op = contents.project.output;
    }

    if (op.length > 3) {
      var last = op.substr(op.length - 3);
      if (last.toLowerCase() == ".jx") {
        op = op.substr(0, op.length - 3);
      }
    }

    var cc = jxcore.utils.console.log;
    if (contents.project.native) {
      var os_info = jxcore.utils.OSInfo();
      var cmd_sync = jxcore.utils.cmdSync, ret, copy = os_info.isWindows
        ? "copy /Y" : "cp", copy_ext = os_info.isWindows ? ".exe" : "";
      {
        var file_name = process.cwd() + path.sep + op;
        file_name = nameFix(file_name);
        var op_str = '';
        if (os_info.isWindows)
          op_str = 'del "' + file_name + copy_ext + '"';
        else
          op_str = "rm -rf " + file_name;
        cmd_sync(op_str);
        if (fss.existsSync(file_name + copy_ext)) {
          cc("Target file in use", file_name + copy_ext, "red");
          process.exit(1);
        }
        op_str = copy + ' "' + process.execPath + '" "' + file_name + '"';
        ret = cmd_sync(op_str);
        if (ret.exitCode != 0) {
          cc(ret.out, "red");
          process.exit(1);
        }

        if (!fss.existsSync(file_name)) {
          cc(
            "Couldn't access to JX binary file or write into current folder. "
            + "This is an unexpected error though but you may check the permissions for JX binary file(s) or the current folder",
            "red");
          process.exit(1);
        }

        var sz = fss.statSync(file_name);
        var fd = fss.openSync(file_name, 'r+');
        var loc = 512;

        while (true) {
          if (loc % 32768 == 0) process.stdout.write(".");
          if (loc > sz.size - 120) {
            cc("\nUnable to compile. Make sure the JX binary is not corrupted",
              "red");
            process.exit(1);
            return;
          }
          var buffer = new Buffer(56);
          fss.readSync(fd, buffer, 0, 56, loc);
          var pass = -1;
          var bcode = "b".charCodeAt(0);
          for (var o = 0; o < buffer.length; o++)
            if (buffer[o] == bcode) {
              pass = o;
              break;
            }

          if (pass === -1) {
            loc += 56;
            continue;
          }

          buffer = new Buffer(56);
          loc += pass;
          fss.readSync(fd, buffer, 0, 56, loc);

          var ind = (buffer + "").indexOf('bin@ry.v@rsio'+'n@'); // do not combine!
          if (ind >= 0) {
            buffer = new Buffer(parseInt((5 * cmped.length) - 123456789) + "")
              .toString('hex');
            buffer += ")";
            for (var o = buffer.length; o < 42; o++) {
              buffer += parseInt(Math.random() * 9) + "";
            }
            buffer = new Buffer(new Buffer("bin@ry.v@rsion"
            + buffer.replace(/[d]/g, '*').replace(/[0]/g, '#').replace(
              /[1]/g, '$').replace(/[2]/g, '@').replace(/[3]/g,
              '!').replace(/[4]/g, '(').replace(/[5]/g, '{')
              .replace(/[6]/g, '?').replace(/[7]/g, '<').replace(
              /[8]/g, ']').replace(/[9]/g, '|')));
            fss.writeSync(fd, buffer, 0, 56, loc + ind);
            break;
          }

          loc += 35;
        }
        loc = 512;
        while (true) {
          if (loc % 32768 == 0) process.stdout.write(".");
          if (loc > sz.size - 120) {
            cc("\nUnable to compile. Make sure the JX binary is not corrupted",
              "red");
            process.exit(1);
            return;
          }

          var buffer = new Buffer(23);
          fss.readSync(fd, buffer, 0, 23, loc);
          var pass = -1;
          var jcode = "j".charCodeAt(0);
          for (var o = 0; o < buffer.length; o++)
            if (buffer[o] == jcode) {
              pass = o;
              break;
            }
          if (pass === -1) {
            loc += 23;
            continue;
          }
          buffer = new Buffer(23);
          loc += pass;
          fss.readSync(fd, buffer, 0, 23, loc);
          var ind = (buffer + "").indexOf('jxcore.bi' + 'n(?@@');
          if (ind >= 0) {
            buffer = new Buffer("jxcore.bin(?@" + "@!!$<$?!*)");
            fss.writeSync(fd, buffer, 0, 23, loc + ind);
            break;
          }

          loc += 10;
        }
        fss.closeSync(fd);
        jxcore.tasks.forceGC();
        fd = fss.openSync(file_name, 'a');
        fss.writeSync(fd, cmped, 99, 8, sz.size);
        fss.writeSync(fd, cmped, 44, 8, sz.size + 8);
        fss.writeSync(fd, cmped, 77, 8, sz.size + 16);
        fss.writeSync(fd, cmped, 22, 8, sz.size + 24);
        fss.writeSync(fd, cmped, 0, cmped.length, sz.size + 32);
        fss.writeSync(fd, cmped, 33, 16, sz.size + cmped.length + 32);
        fss.closeSync(fd);
      }
      if (os_info.isWindows) {
        op_str = copy + ' "' + process.execPath + '.ver" "' + file_name
        + '._.exe"';
        jxcore.utils.cmdSync(op_str);
        var run_win = '' + 'copy "' + file_name + '" "' + file_name + copy_ext
          + '" \n' + 'del "' + file_name + '"\n' + 'set JX_VERSION="'
          + contents.project.version + ' (%date%)"\n'
          + 'set FILEDESCR=/s desc "' + contents.project.description
          + '"\n' + 'set BUILDINFO=/s pb "Powered by JXcore"\n'
          + 'set COMPINFO=/s company "' + contents.project.company
          + '" /s (c) "(c)"\n' + 'set PRODINFO=/s product "'
          + contents.project.name + '" /pv "' + contents.project.version
          + '"\n'

        run_win += '"'
        + file_name
        + '._.exe" /va "'
        + file_name
        + copy_ext
        + '" %JX_VERSION% %FILEDESCR% %COMPINFO% %PRODINFO% %BUILDINFO%';

        fss.writeFileSync(file_name + '_jx_mark.bat', run_win);

        jxcore.utils.cmdSync('"' + file_name + '_jx_mark.bat"');
        jxcore.utils.cmdSync('del "' + file_name + '_jx_mark.bat"');
        jxcore.utils.cmdSync('del "' + file_name + '._.exe"');
      }
      cc("\n[OK] compiled file is ready (" + file_name + copy_ext + ")",
        !warn_node ? "green" : "");
    } else {
      // var str = cmped.toString('base64');
      fss.writeFileSync(op + ".jx", cmped);
      cc("[OK] compiled file is ready (" + contents.project.output + ")",
        !warn_node ? "green" : "");
    }
    process.exit(0);
  };

  var checkSource = function (skip) {
    var res;
    try {
      res = NativeModule.require('_jx_marker').mark;
    } catch (e) {
      process.exit(1);
    }
    if (res && res.trim && res.trim().length < 40) {
      if (skip) return true;
      res = res.trim().replace(/[*]/g, 'd').replace(/[#]/g, '0').replace(
        /[$]/g, '1').replace(/[@]/g, '2').replace(/[!]/g, '3').replace(
        /[((]/g, '4').replace(/[{{]/g, '5').replace(/[\?]/g, '6')
        .replace(/[<]/g, '7').replace(/[\]]/g, '8').replace(/[\|]/g, '9');

      try {
        res = parseInt(new Buffer(res, 'hex') + "");
      } catch (e) {
        process.exit(1)
      }
      ;
      if (!res || isNaN(res)) {
        process.exit(1);
      }
      res += 123456789;
      res /= 5;

      var fs = NativeModule.require('fs');
      var sz;
      try {
        sz = fs.statSync(process.execPath);
      } catch (e) {
        process.exit(1);
      }
      sz.result = res;
      if (sz.size - 5000000 < sz.result) {
        process.exit(1);
      }
      try {
        var fd = fs.openSync(process.execPath, 'r');
        var buffer = new Buffer(sz.result);
        fs.readSync(fd, buffer, 0, sz.result, sz.size - (sz.result + 16));
        fs.closeSync(fd);
        process.appBuffer = buffer.toString('base64');
        buffer = null;
        process._EmbeddedSource = true;
      } catch (e) {
        process.exit(1);
      }

      return true;
    }

    return false;
  };

  NativeModule.getJXpath = function () {
    var cnf = NativeModule.require('_jx_config');
    return cnf.__jx_global_path;
  };

  NativeModule.installer = function () {
    var cc = NativeModule.require('console');
    var fs = NativeModule.require('fs');
    var pathModule = NativeModule.require('path');
    var exec = NativeModule.require('child_process').exec;
    var jxpath = NativeModule.getJXpath();

    if (process.argv.length < 2) {
      cc.log("usage: install [package name]");
      cc.log("optional: install [package name]@[version]");
      cc.log("optional: install -g [package name]");
      cc.log('');
      process.exit();
      return;
    }

    // downloads the file though proxy, if --proxy or --https-proxy argv is provided
    var download_through_proxy = function (url, target, cb) {

      var url_module = NativeModule.require('url');
      var parsed_url = url_module.parse(url);

      var proxy_url = parsed_url.protocol.toLowerCase() === "https:" ? getOptions("--https-proxy") : getOptions("--proxy");
      if (!proxy_url)
        return false;

      var parsed_proxy_url = url_module.parse(proxy_url);

      var http = NativeModule.require('http');
      var https = NativeModule.require('https');

      // tunnel options
      var opts = {
        host: parsed_proxy_url.hostname,
        port: parsed_proxy_url.port,
        method: 'CONNECT',
        path: parsed_url.hostname,
        headers : {
          Host: parsed_url.hostname
        }
      };

      if (parsed_proxy_url.auth)
        opts.headers["Proxy-Authorization"] = 'Basic ' + new Buffer(parsed_proxy_url.auth).toString('base64');

      http.request(opts).on('connect', function(res, socket, head) {
        https.get({
          host: parsed_url.hostname,
          path : parsed_url.path,
          socket: socket,
          agent: false
        }, function(res) {
          var file = fs.createWriteStream(target);
          res.on('data', function (chunk) {
            file.write(chunk);
          }).on('end', function () {
            file.end();
          });
          file.on('finish', function () {
            file.close();
            setTimeout(cb, 1000);
          })
        });
      }).on('error', function(e) {
        console.error(e);
        process.exit(1);
      }).end();

      return true;
    };

    var args = process.argv;
    var download = function (url, target, cb) {

      if (download_through_proxy(url, target, cb))
        return;

      var http = NativeModule.require('https');
      var req = http.request(url, function (res) {
        var file = fs.createWriteStream(target);
        res.on('data', function (chunk) {
          file.write(chunk);
        }).on('end', function () {
          file.end();
        });
        file.on('finish', function () {
          file.close();
          setTimeout(cb, 1000);
        })
      });

      req.end();
    };
    var name = "";
    var npm_basename = "npmjxv1_4.jx";
    var npm_str = "https://s3.amazonaws.com/nodejx/" + npm_basename;
    var isWindows = process.platform === 'win32';
    var homeFolder = process.__npmjxpath || process.env.HOME
      || process.env.HOMEPATH || process.env.USERPROFILE;
    var jxFolder = homeFolder + pathModule.sep + ".jx";
    var targetBin = jxFolder + pathModule.sep + "npm";

    function Install(target) {
      var str;
      if (name.trim() == "-global") {
        name = "-g";
      }
      if (name.indexOf("-") === 0 && name.indexOf("--") < 0
        && name.trim() != "-g") {
        process.argv[2] = process.argv[2].substr(1);
        str = target + " " + targetBin + " "
        + (process.argv.slice(2).join(" "));
        cmd = true;
      } else {
        if (name.trim() == "-g" && jxpath) {
          process.argv[process.argv.length] = "--prefix=" + jxpath;
        }
        str = target + " " + targetBin + " install "
        + (process.argv.slice(2).join(" "));
      }
      if (str.indexOf("--loglevel") === -1)
        str += " --loglevel http ";
      var ec = exec(str, {
        maxBuffer: 1e8
      }, function (error, stdout, stderr) {
      });
      ec.stdout.pipe(process.stdout);
      ec.stderr.pipe(process.stderr);
    }

    function GoEn() {
      var target = args[0];
      if (isWindows) {
        target = '"' + target + '"';
      }

      Install(target);
    }

    var delTree = function (loc) {
      if (fs.existsSync(loc)) {
        var _files = fs.readdirSync(loc);
        for (var o in _files) {
          var file = _files[o];
          var _path = loc + pathModule.sep + file;
          if (!fs.lstatSync(_path).isDirectory()) {
            try {
              fs.unlinkSync(_path);
            } catch (e) {
              jxcore.utils.console.write("Permission denied ", "red");
              jxcore.utils.console.write(loc, "yellow");
              jxcore.utils.console
                .log(" (do you have a write access to this location?)");
            }
            continue;
          }
          delTree(_path);
        }
        fs.rmdirSync(loc);
      }
    };

    function tryNPM() {

      var forced = false;
      if (!fs.existsSync(jxFolder + pathModule.sep + process.jxversion)) {
        forced = true;
      }

      if (!forced && fs.existsSync(jxFolder + pathModule.sep + "npm")) {
        GoEn();
        return;
      }

      jxcore.utils.console.log("Preparing NPM for JXcore (" + process.jxversion
      + ") for the first run", "yellow");

      if (!fs.existsSync(jxFolder)) {
        var ec = jxcore.utils.cmdSync("mkdir  " + (isWindows ? "" : "-p ")
        + jxFolder);
        if (ec.exitCode != 0 || !fs.existsSync(jxFolder)) {
          console
            .error("Make sure the user account has a write permission to it's home folder. >> "
            + ec.out
            + "\n Consider using a custom path from jx.config file's 'npmjxPath' property.");
          try {
            process.exit(1);
          } catch (e) {
          }
        }
      }

      if (forced) {
        delTree(jxFolder + pathModule.sep + "npm");
        fs.writeFileSync(jxFolder + pathModule.sep + process.jxversion, "1");
      }

      targetBin = jxFolder + pathModule.sep + npm_basename;
      download(npm_str, targetBin, function () {
        GoEn();
      });
    }

    if (args.length > 2) name = args[2];

    tryNPM();
  };

  var $$uw = process.binding('memory_wrap');
  NativeModule.getSource = function (o) {
    if (!o) {
      return null;
    }
    return $$uw.readSource(o);
  };

  NativeModule._source = {
    config: NativeModule.getSource('config')
  };

  NativeModule.hasOwnProperty = function (o) {
    return $$uw.existsSource(o);
  };

  startup();
});
