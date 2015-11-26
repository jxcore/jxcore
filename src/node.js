// Copyright & License details are available under JXCORE_LICENSE file

(function(process) {
  this.global = this;

  if (!Error.captureStackTrace) {
    var _stackProto = function(msg, fileName, lineNumber, columnNumber) {
      if (fileName.indexOf('@') >= 0) {
        var spl = fileName.split('@');
        this._fileName = spl[1];
        this._functionName = spl[0];
      } else {
        this._fileName = fileName;
        this._functionName = 'unknown';
      }

      this._lineNumber = lineNumber;
      this._columnNumber = columnNumber;
      this._msg = msg;
    };

    _stackProto.prototype.getFileName = function() {
      return this._fileName;
    };
    _stackProto.prototype.getColumnNumber = function() {
      return this._columnNumber;
    };
    _stackProto.prototype.getLineNumber = function() {
      return this._lineNumber;
    };
    _stackProto.prototype.toString = function() {
      return this._msg;
    };
    _stackProto.prototype.isEval = function() {
      // TODO(obastemur) fix this!
      return false;
    };
    _stackProto.prototype.getFunctionName = function() {
      return this._functionName;
    };

    Error.captureStackTrace = function(err, __) {
      var st;
      if (typeof err === 'undefined' || typeof err === 'string') {
        return;
      }

      if (!err.stack) {
        // TODO(obastemur) there must be a better way to do this
        try {
          throw new Error('');
        } catch (e) {
          err.stack = e.stack;
          err.fileName = e.fileName;
          err.lineNumber = e.lineNumber;
          err.columnNumber = e.columnNumber;
        }

        if (!err.stack)
          err.stack = '\n'; // silly but we don't want to throw here no matter
        // what

        st = err.stack.split('\n');
        st.shift();
      } else {
        if (!Array.isArray(err.stack))
          st = err.stack.split('\n');
      }

      if (Array.isArray(err.stack) && err.stack[0] && err.stack[0]._msg) {
        // TODO(obastemur) bug ? app gives back an object that was already
        // processed
      } else {
        err.stack = new Array();
        var stackLimit = Error.stackTraceLimit ? Error.stackTraceLimit : 9;
        var max = Math.min(stackLimit, st.length);
        for (var i = 0; i < max; i++) {
          var arr = st[i].split(':');
          var msg = '    at ' + st[i];
          if (arr.length == 3) {
            err.stack[i] = new _stackProto(msg, arr[0], arr[1], arr[2]);
          } else {
            err.stack[i] = new _stackProto(msg, err.fileName, err.lineNumber,
                err.columnNumber);
          }
        }

        err.stack.toString = function() {
          var arr = err.stack.join('\n');
          if (arr.length > 4 && arr.substr(arr.length - 3, 2) == 'at')
            arr = arr.substr(0, arr.length - 3);
          if (!err.name)
            err.name = 'Error';
          return err.name + ': ' + err.message + '\n' + arr;
        };
      }

      if (Error.prepareStackTrace) {
        try {
          var newStack = Error.prepareStackTrace(err, __ || err.stack);
          if (newStack)
            err.stack = newStack;
        } catch (e) {
          // silly but do not let Error.prepareStackTrace throwing
        }
      }
    };

    if (!global.gc) {
      global.gc = function() {
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

    if (process.argv[1] !== '--debug-agent')
      startup.processChannel();
    
    startup.processRawDebug();

    startup.resolveArgv0();

    if (!process.subThread) {
      var __exitCode = 0;
      var __pstart = Date.now();
      var __kill = function(code) {
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
      var resetBody = function(code) {
        if (Date.now() - __pstart < 5000) {
          var diff = Date.now() - __pstart;
          console.error(
              'Automatic reset feature is only applicable to applications ' +
              'active for at least 5000 milliseconds');
          console.error('The application was alive for ', diff, 'ms');
          __kill(code);
          return;
        }

        var fs = NativeModule.require('fs');
        var path = NativeModule.require('path');
        var fname = './jxcore.' +
            path.basename(process.mainModule.filename) + '.log';

        if (fs.existsSync(fname))
          fs.unlinkSync(fname);

        var spawn = NativeModule.require('child_process').spawn, out = fs
            .openSync(fname, 'a'), err = fs.openSync(fname, 'a');

        var cmd = process.argv[0];
        var params = process.argv.slice(1);
        params.push('$JX$CORE_APP_RESET');

        var child = spawn(cmd, params, {
          detached: true,
          stdio: ['ignore', out, err]
        });

        child.unref();
        __kill(code);
      };

      process.on('$$restart', function(code) {
        try {
          __exitCode = code;
          if (startup.hasResetCB())
            process.emit('restart', resetBody, code);
          else {
            process.exit(code);
          }
        } catch (e) {
          console.error('process.on -> restart', e);
        }
      });
    }

    var co = NativeModule.require('console');
    var process_restarted = jxcore.utils.argv.remove('$JX$CORE_APP_RESET');
    var parsedArgv = jxcore.utils.argv.parse();

    // There are various modes that Node can run in. The most common two
    // are running from a script and running the REPL - but there are a few
    // others like the debugger or running --eval arguments. Here we decide
    // which mode we run in.

    var getActiveFolder = function() {
      var sep;
      try {
        sep = process.cwd() + NativeModule.require('path').sep;
      } catch (e) {
        co.error(
            'Perhaps the path you are running on is not ' +
            'exist any more. Please revisit the path and try again.');
        process.exit(1);
      }
      return sep;
    };

    var isPackaged = checkSource(process.subThread);
    if (process.isPackaged != isPackaged) {
      if (process.subThread) {
        process.isPackaged = isPackaged;
      } else {
        co.error('Binary corrupted');
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
      npm: false,
      compile: false,
      package: false,
      packagetojx: false
    };

    if (!process.isEmbedded && !process._EmbeddedSource && !process.subThread) {
      for (var o in __ops) {
        if (__ops.hasOwnProperty(o) && process.argv[1] === o) {
          var sep = getActiveFolder();
          __ops[o] = !NativeModule.require('fs').existsSync(sep + o + '.js');
          if (!__ops[o]) {
            co.error(
                'while [jx ' + o +
                '] is a special command, the current folder has ' +
                o + '.js, running that instead.\n');
          }
          break;
        }
      }
    }

    var Module = NativeModule.require('module');

    if (__ops.compile) {
      if (!process.argv[2]) {
        jxcore.utils.console.log('JXcore Packaging System');
        co.log('please provide the jxp file to compile');
        co.log('usage: compile [jxp file to compile]');
        co.log('');
        return;
      }
      co.log('Processing the project file..');
      NativeModule.require('_jx_package').compile(process.argv, __ops);
      return;
    } else if (__ops.package || __ops.packagetojx) {
      if (process.argv.length < 3) {
        jxcore.utils.console.log('J' + 'Xcore Packaging System');
        co.log('usage: package [main javascript file] [project name]');
        co.log('');
        process.exit();
        return;
      }
      co.log('Processing the folder..');
      NativeModule.require('_jx_package').package(process.argv);
      return;
    } else if (__ops.install || __ops.npm) {
      NativeModule.require('_jx_install').install();
      return;
    } else if (process.argv[1] == 'debug') {
      // Start the debugger agent
      var d = NativeModule.require('_debugger');
      d.start();
    } else if (process.argv[1] == '--debug-agent') {
      // Start the debugger agent
      var d = NativeModule.require('_debugger_agent');
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
        __debug = global.v8debug && process.execArgv.some(function(arg) {
          return arg.match(/^--debug-brk(=[0-9]*)?$/);
        });

        var mter = parsedArgv.mt || parsedArgv['mt-keep'];
        if (mter) {
          var mterCount = mter.isInt ? mter.asInt : -1;
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
        } else if (!process.subThread &&
                   process.argv[1].toLowerCase() == 'monitor') {
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
        }
      } else {
        NativeModule.require('_jx_config');
      }

      if (!process.subThread && !process._Monitor) {
        // If this is a worker in cluster mode, start up the communication
        // channel.
        if (process.env.NODE_UNIQUE_ID) {
          var cluster = NativeModule.require('cluster');

          cluster._setupWorker();

          // Make sure it's not accidentally inherited by child processes.
          delete process.env.NODE_UNIQUE_ID;
        }
      }

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

      if (process.isPackaged && process.env.JX_MONITOR_RUN) {
        var monHelper = NativeModule.require('_jx_monitorHelper');
        // if app dies before start_delay, that's ok: following will not take
        // place
        monHelper.tryToFollowMeOrExit();
      }

    } else {
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
        repl.on('exit', function() {
          process.exit();
        });

      } else {
        // Read all of stdin - execute it.
        process.stdin.setEncoding('utf8');

        var code = '';
        process.stdin.on('data', function(d) {
          code += d;
        });

        process.stdin.on('end', function() {
          process._eval = code;
          evalScript('[stdin]');
        });
      }
    }
  }

  startup.globalVariables = function() {
    global.process = process;
    global.global = global;
    global.GLOBAL = global;
    global.root = global;
    global.Buffer = NativeModule.require('buffer').Buffer;
    process.binding('buffer').setFastBufferConstructor(global.Buffer);
    process.domain = null;
    process._exiting = false;
    var tw = process.binding('thread_wrap');

    process.sendToMain = function(obj) {
      if (process.__reset)
        return;

      if (!process.subThread)
        jxcore.tasks.emit('message', -1, obj);
      else
        tw.sendToAll(-1, JSON.stringify({
          threadId: process.threadId,
          params: obj
        }), process.threadId);
    };

    process.sendToThread = function(threadId, obj) {
      if (process.__reset)
        return;
      if (threadId == null || threadId == undefined) {
        throw new TypeError('threadId must be defined');
      }

      if (threadId < -1 || threadId > 63) {
        throw new RangeError('threadId must be between -1 and 63');
      }
      tw.sendToAll(threadId, JSON.stringify({
        tid: process.threadId,
        data: obj
      }), process.threadId);
    };

    process.sendToThreads = function(obj) {
      if (process.__reset)
        return;

      tw.sendToAll(-2, JSON.stringify({
        tid: process.threadId,
        data: obj
      }), process.threadId);
    };

    process.keepAlive = function() {
      // DUMMY
      console.warn("process.keepAlive shouldn't be called from main thread");
    };

    process.release = function() {
      // DUMMY
      console.warn("process.release shouldn't be called from main thread");
    };

    process.unloadThread = function() {
      // DUMMY
      console.warn("process.unloadThread shouldn't be called from main thread");
    };
  };

  startup.globalTimeouts = function() {
    global.setTimeout = function() {
      var t = NativeModule.require('timers');
      return t.setTimeout.apply(this, arguments);
    };

    global.setInterval = function() {
      var t = NativeModule.require('timers');
      return t.setInterval.apply(this, arguments);
    };

    global.clearTimeout = function() {
      var t = NativeModule.require('timers');
      return t.clearTimeout.apply(this, arguments);
    };

    global.clearInterval = function() {
      var t = NativeModule.require('timers');
      return t.clearInterval.apply(this, arguments);
    };

    global.setImmediate = function() {
      var t = NativeModule.require('timers');
      return t.setImmediate.apply(this, arguments);
    };

    global.clearImmediate = function() {
      var t = NativeModule.require('timers');
      return t.clearImmediate.apply(this, arguments);
    };
  };

  startup.globalConsole = function() {
    global.__defineGetter__('console', function() {
      return NativeModule.require('console');
    });
  };

  startup.globalJXcore = function() {
    global.__defineGetter__('jxcore', function() {
      var obj = {};
      obj.__defineGetter__('utils', function() {
        return NativeModule.require('_jx_utils');
      });
      obj.__defineGetter__('store', function() {
        return NativeModule.require('_jx_memStore').store;
      });
      obj.__defineGetter__('tasks', function() {
        return NativeModule.require('_jx_tasks');
      });
      obj.__defineGetter__('monitor', function() {
        return NativeModule.require('_jx_monitor');
      });
      obj.__defineGetter__('embeddedModule', function() {
        return NativeModule.require('_jx_loadEmbedded');
      });
      obj.__defineGetter__('uwp', function() {
        return NativeModule.require('_jx_loadEmbedded').require('node-uwp');
      });
      return obj;
    });
  };

  startup._lazyConstants = null;

  startup.lazyConstants = function() {
    if (!startup._lazyConstants) {
      startup._lazyConstants = process.binding('constants');
    }
    return startup._lazyConstants;
  };

  startup.processFatal = function() {
    // call into the active domain, or emit uncaughtException,
    // and exit if there are no listeners.
    process._fatalException = function(er) {
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
        if (domain._disposed)
          return true;

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
          if (domain === domainModule.active)
            domainStack.pop();
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
  startup.processAssert = function() {
    // Note that calls to assert() are pre-processed out by JS2C for the
    // normal build of node. They persist only in the node_g build.
    // Similarly for debug().
    assert = process.assert = function(x, msg) {
      if (!x)
        throw new Error(msg || 'assertion error');
    };
  };

  startup.processConfig = function() {
    // used for `process.config`, but not a real module
    var config = NativeModule._source.config;
    delete NativeModule._source.config;

    // strip the gyp comment line at the beginning
    config = config.split('\n').slice(1).join('\n').replace(/'/g, '"');

    process.config = JSON.parse(config, function(key, value) {
      if (value === 'true')
        return true;
      if (value === 'false')
        return false;
      return value;
    });
  };

  process.dlopen = function(module, filename) {
    var isWindows = process.platform === 'win32' || process.platform === 'winrt';

    if (isWindows) {
      filename = filename.toLowerCase();
    }

    var gpath = process._dlopen(module, filename);
    if (gpath) {
      var fs = NativeModule.require('fs');
      var pathModule = NativeModule.require('path');
      var err = new Error(
          'On this system, processes are limited to run native modules only ' +
          'from global repository. ' +
          filename + " wasn't exist. " +
          "Please make sure it's a standard JXcore / node.js native module.");
      var i = filename.lastIndexOf('node_modules');
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

  startup.processNextTick = function() {
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
        if (now - last_time_maxTickWarn < 2)
          return;
        last_time_maxTickWarn = now;
      }

      var msg = '(node) warning: Recursive process.nextTick detected. ' +
          'This will break in the next version of node. ' +
          'Please use setImmediate for recursive deferral.';

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
      if (infoBox[depth] !== 0)
        infoBox[depth] = 0;
      // no callbacks to run
      if (infoBox[length] === 0)
        return infoBox[index] = infoBox[depth] = 0;
      process._tickCallback();
    }

    // run callbacks that have no domain
    // using domains will cause this to be overridden
    function _tickCallback() {
      var callback, nextTickLength, threw;

      if (inTick)
        return;
      if (infoBox[length] === 0) {
        infoBox[index] = 0;
        infoBox[depth] = 0;
        return;
      }
      inTick = true;

      while (infoBox[depth]++ < process.maxTickDepth) {
        nextTickLength = infoBox[length];
        if (infoBox[index] === nextTickLength)
          return tickDone(0);

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

      if (inTick)
        return;
      inTick = true;

      // always do this at least once. otherwise if process.maxTickDepth
      // is set to some negative value, or if there were repeated errors
      // preventing depth from being cleared, we'd never process any
      // of them.
      while (infoBox[depth]++ < process.maxTickDepth) {
        nextTickLength = infoBox[length];
        if (infoBox[index] === nextTickLength)
          return tickDone(0);

        while (infoBox[index] < nextTickLength) {
          tock = nextTickQueue[infoBox[index]++];
          callback = tock.callback;
          if (tock.domain) {
            if (tock.domain._disposed)
              continue;
            tock.domain.enter();
          }
          threw = true;
          try {
            callback();
            threw = false;
          } finally {
            // finally blocks fire before the error hits the top level,
            // so we can't clear the depth at this point.
            if (threw)
              tickDone(infoBox[depth]);
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
      if (process._exiting)
        return;
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
      if (process._exiting)
        return;

      if (infoBox[depth] >= process.maxTickDepth)
        maxTickWarn();

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
      script = 'global.__filename = ' + JSON.stringify(name) + ';\n' +
               'global.exports = exports;\n' +
               'global.module = module;\n' +
               'global.__dirname = __dirname;\n' +
               'global.require = require;\n' +
               'return require("vm").runInThisContext(' +
               JSON.stringify(body) + ', ' +
               JSON.stringify(name) + ', true);\n';
    }

    var result = module._compile(script, name + '-wrapper');
    if (process._print_eval)
      console.log(result);
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
  
  startup.processRawDebug = function() {
    var format = NativeModule.require('util').format;
    var rawDebug = process._rawDebug;
    process._rawDebug = function() {
      rawDebug(format.apply(null, arguments));
    };
  };

  startup.processStdio = function() {
    var stdin, stdout, stderr;

    var util = NativeModule.require('util');
    var isSTD = (process.platform === 'android' || process.platform == 'winrt')
                  && process.isEmbedded;
    var $tw;
    if (isSTD) {
      $tw = process.binding('jxutils_wrap');
    }

    var fake_stdout = null, fake_stderr = null;
    var fake_stdin = null;

    var Writable = NativeModule.require('stream').Writable;
    util.inherits(StdLogCatOut, Writable);

    function StdLogCatOut(opt) {
      Writable.call(this, opt);
    }

    if (isSTD) { // target LogCat for stdout and stderr
      fake_stdout = new StdLogCatOut();
      fake_stdout.write = fake_stdout._write = function(bf) {
        $tw.print(bf + '');
      };
      
      fake_stdin = new StdLogCatOut();
      fake_stdin.read = fake_stdin._read = function(bf) {
        return new Buffer("");
      };

      fake_stderr = new StdLogCatOut();
      fake_stderr.write = fake_stderr._write = function(bf) {
        $tw.print_err_warn(bf + '', true);
      };
    }

    process.__defineGetter__('stdout', function() {
      if (stdout)
        return stdout;
      if (isSTD) {
        stdout = fake_stdout;
      } else {
        stdout = createWritableStdioStream(1);
      }

      stdout.destroy = stdout.destroySoon = function(er) {
        er = er || new Error('process.stdout cannot be closed.');
        stdout.emit('error', er);
      };

      if (stdout.isTTY) {
        process.on('SIGWINCH', function() {
          stdout._refreshSize();
        });
      }

      return stdout;
    });

    process.__defineGetter__('stderr', function() {
      if (stderr)
        return stderr;

      if (isSTD) {
        stderr = fake_stderr;
      } else {
        stderr = createWritableStdioStream(2);
      }

      stderr.destroy = stderr.destroySoon = function(er) {
        er = er || new Error('process.stderr cannot be closed.');
        stderr.emit('error', er);
      };

      return stderr;
    });

    process.__defineGetter__('stdin', function() {
      if (stdin)
        return stdin;

      if (process.isEmbedded && (isSTD || process.platform == 'ios')) {
        console.error('stdin is not supported on embedded applications');
        stdin = fake_stdin;
        // do not throw or return null
      } else {
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
      stdin.on('pause', function() {
        if (!stdin._handle)
          return;
        stdin._readableState.reading = false;
        stdin._handle.reading = false;
        stdin._handle.readStop();
      });

      return stdin;
    });

    process.openStdin = function() {
      process.stdin.resume();
      return process.stdin;
    };
  };

  startup.processKillAndExit = function() {
    process.exit = function(code) {
      if (!process._exiting) {
        process._exiting = true;
        process.emit('exit', code || 0);
      }
      process.reallyExit(code || 0);
    };

    process.kill = function(pid, sig) {
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

  startup.processSignalHandlers = function() {
    // Load events module in order to access prototype elements on process like
    // process.addListener.
    var signalWraps = {};
    var addListener = process.addListener;
    var removeListener = process.removeListener;

    function isSignal(event) {
      return event.slice(0, 3) === 'SIG' &&
             startup.lazyConstants().hasOwnProperty(event);
    }

    startup.hasResetCB = function() {
      return hasRestartListener;
    };

    // Wrap addListener for the special signal types
    process.on = process.addListener = function(type, listener) {
      if (type == 'restart') {
        hasRestartListener = true;
      }

      if (isSignal(type) && !signalWraps.hasOwnProperty(type)) {
        var Signal = process.binding('signal_wrap').Signal;
        var wrap = new Signal();

        wrap.unref();

        wrap.onsignal = function() {
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

    process.removeListener = function(type, listener) {
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

  startup.processChannel = function() {
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
  };

  startup.resolveArgv0 = function() {
    var cwd;
    try {
      cwd = process.cwd();
    } catch (e) {
      console.error(
          'Error: You may not have a read access on current folder or a ' +
          'file system link to current folder removed. ' +
          'Please revisit the folder and make sure you have an access.');
      process.exit(1);
    }
    
    var isWindows;
    if (process.platform === 'winrt') {
      isWindows = true;
      process.userPath = cwd;
    } else {
      isWindows = process.platform === 'win32';
    }

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

  NativeModule.require = function(id) {
    if (!id) {
      throw new TypeError('NativeModule.require expects name of the module');
    }

    if (id == 'native_module') {
      return NativeModule;
    }

    var cached = NativeModule.getCached(id);
    if (cached) {
      return cached.exports;
    }

    if (!NativeModule.exists(id)) {
      throw new Error('No such native module ' + id);
    }

    if (id.indexOf('_jx_') < 0) {
      process.moduleLoadList.push('NativeModule ' + id);
    }

    var nativeModule = new NativeModule(id);

    nativeModule.cache();
    nativeModule.compile();

    return nativeModule.exports;
  };

  NativeModule.getCached = function(id) {
    if (NativeModule._cache.hasOwnProperty(id)) {
      return NativeModule._cache[id];
    } else {
      return null;
    }
  };

  NativeModule.exists = function(id) {
    if (id == 'config')
      return false;
    return NativeModule.hasOwnProperty(id);
  };

  NativeModule.wrap = function(script) {
    return NativeModule.wrapper[0] + script + NativeModule.wrapper[1];
  };

  NativeModule.wrapper = [
    '(function (exports, require, module, __filename, __dirname, ' +
        'setTimeout, setInterval, process) { ',
    '\n});'];

  NativeModule.prototype.compile = function() {
    var source = NativeModule.getSource(this.id);
    source = NativeModule.wrap(source, this.id === 'module');

    var fn = runInThisContext(source, this.filename, true, 0);

    fn(this.exports, NativeModule.require, this, this.filename, undefined,
        global.setTimeout, global.setInterval, global.process);

    this.loaded = true;
  };

  NativeModule.prototype.cache = function() {
    NativeModule._cache[this.id] = this;
  };

  var checkSource = function(skip) {
    var res;
    try {
      res = NativeModule.require('_jx_marker').mark;
    } catch (e) {
      process.exit(1);
    }
    if (res && res.trim && res.trim().length < 40) {
      if (skip)
        return true;
      res = res.trim().replace(/[*]/g, 'd').replace(/[#]/g, '0').replace(
          /[$]/g, '1').replace(/[@]/g, '2').replace(/[!]/g, '3').replace(
          /[((]/g, '4').replace(/[{{]/g, '5').replace(/[\?]/g, '6').replace(
          /[<]/g, '7').replace(/[\]]/g, '8').replace(/[\|]/g, '9');

      try {
        res = parseInt(new Buffer(res, 'hex') + '');
      } catch (e) {
        process.exit(1);
      }

      if (!res || isNaN(res)) {
        process.exit(1);
      }
      res += 123456789;
      res /= 5;

      var fs = NativeModule.require('fs');
      try {
        var fd = fs.openSync(process.execPath, 'r');
		var checkBuffer = new Buffer(16), buffer;
		fs.readSync(fd, checkBuffer, 0, 16, res);
		if(checkBuffer[15] === 0xff) {
			buffer = new Buffer(
				(checkBuffer[7] << 24) + // Data size
				(checkBuffer[8] << 16) +
				(checkBuffer[9] << 8) +
				checkBuffer[10]);
			fs.readSync(fd, buffer, 0,
				buffer.length
				, res + 16 + // Data offset
				(checkBuffer[11] << 24) +
				(checkBuffer[12] << 16) +
				(checkBuffer[13] << 8) +
				checkBuffer[14]
				);
			fs.closeSync(fd);
			process.appBuffer = buffer.toString('base64');
			buffer = null;
			process._EmbeddedSource = true;
			}
		else process.exit(1);
      } catch (e) {
        process.exit(1);
      }

      return true;
    }

    return false;
  };

  var $$uw = process.binding('memory_wrap');
  NativeModule.getSource = function(o) {
    if (!o) {
      return null;
    }
    return $$uw.readSource(o);
  };

  NativeModule._source = {
    config: NativeModule.getSource('config')
  };

  NativeModule.hasOwnProperty = function(o) {
    return $$uw.existsSource(o);
  };

  startup();
});
