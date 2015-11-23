// Copyright & License details are available under JXCORE_LICENSE file

var tw = process.binding('thread_wrap');
var ex_data = process.binding('memory_wrap');
var path = require('path');

var strPath = (process.argv.length > 1) ? process.argv[1] : null;
var mter = false, MTnoKeep = false;

// inner methods are internal use only
var inner = global.tools;
delete global.tools;

var resolved = ex_data.readSource('_EmbeddedSource.mainModule.filename');
var resolved_set = false;

if (resolved) {
  resolved_set = true;
  require.main.filename = resolved;
}

// When embedded, embedding application provides the entry point via
// process.argv
// sub thread should be relying on the paths given by the embedder
if (process.isEmbedded) {
  resolved = path.join(path.dirname(process.argv[0]), process.argv[1]);
} else if (strPath && !process.isPackaged) {
  mter = (strPath.indexOf('mt:') >= 0 || strPath == 'mt');
  MTnoKeep = mter;
  if (!mter) {
    mter = strPath == 'mt-keep' || strPath.indexOf('mt-keep:') >= 0;
  }

  if (mter) {
    process.argv[2] = path.resolve(process.argv[2]);
    if (!resolved) resolved = process.argv[2];
  }
  else {
    process.argv[1] = path.resolve(process.argv[1]);
    if (!resolved) resolved = process.argv[1];
  }

  if (process.argv[process.argv.length - 1] == undefined) {
    process.argv = process.argv.slice(0, process.argv.length - 1);
  }
}

if (!resolved_set) {
  require.main.filename = resolved;
}

if (process.reloadModules) {
  process.reloadModules();
  delete(process.reloadModules);
}

if (require.main.fileSource)
  require.main.fileSource = null;

var _waitCounter = 0;
var setWaitCounter = function(counter) {
  if (counter === _waitCounter)
    return;
  _waitCounter = counter;
  inner.refWaitCounter(counter);
};

var incWaitCounter = function() {
  setWaitCounter(_waitCounter + 1);
};

var decWaitCounter = function() {
  setWaitCounter(_waitCounter - 1);
};

var getWaitCounter = function() {
  return _waitCounter;
};

process.keepAlive = function(ms) {
  if (process.__reset)
    return;
  incWaitCounter();
  tw.sendToAll(-1,
      JSON.stringify({threadId: process.threadId, params: null, wait: 1}),
      process.threadId);
  if (ms) {
    setTimeout(function() {
      process.release();
    }, ms, 1);
  }
};

var restarted = false;

process.release = function(customCounter) {
  if (getWaitCounter() <= 0)
    return;
  if (!process.__reset)
    customCounter = null;

  var num = customCounter ? customCounter : 1;
  if (!restarted && !process.__reset) {
    setTimeout(function() {
      if (process.__reset) {
        return;
      }
      if (getWaitCounter() < num) {
        num = getWaitCounter();
      }
      if (num == 0) {
        return;
      }
      tw.sendToAll(-1,
          JSON.stringify({
            threadId: process.threadId,
            params: null,
            wait: -1 * num}),
          process.threadId);
      setWaitCounter(getWaitCounter() - num);
    }, 50, 2); // hack
  }
  else {
    if (getWaitCounter() < num) {
      num = getWaitCounter();
    }
    if (num == 0) {
      return;
    }
    tw.sendToAll(-1,
        JSON.stringify({
          threadId: process.threadId,
          params: null,
          wait: -1 * num}),
        process.threadId);
    setWaitCounter(getWaitCounter() - num);
  }
};

var embedFunc = function(_globals, fbody, methodBody, waitLogic) {
  var def = fbody.toString();
  var n = def.lastIndexOf('}');

  if (waitLogic) {
    methodBody += 'exports.waitFirst = true;';
  }
  return _globals + '(' + def.substr(0, n) + '\n' + methodBody + '\n})();';
};

var isWindows = process.platform === 'win32';

var strGlobals = "/**/__dirname = '" + path.dirname(resolved) +
    "';__filename = '" + resolved +
    "';process.mainModule.filename='" + resolved + "';";

if (isWindows) {
  strGlobals = strGlobals.replace(/\\/g, '\\\\');
}

var func = function(scr) {
  var name = '~NodeJXThread-' + scr[0];
  var ind = scr[1].indexOf('$$&JX;1$node$&JX;$');

  if (ind < 0) {
    ex_data.setMap(process.threadId, '*' + name,
        strGlobals + 'var ___method=' + scr[1] + ';' +
        'exports.call = function(___cbid, ___param){' +
        'var ___x=___method(___param);' +
        'if(___x === undefined || ___cbid==-1)' +
        '{return JSON.stringify({"_id":___cbid, dummy:true})};' +
        'return JSON.stringify({"_id":___cbid, "o":___x});' +
        '}');
  } else {
    var strl = scr[1].substr(0, ind);
    var strd = scr[1].replace(strl + '$$&JX;1$node$&JX;$', '').trim();
    var sub2 = false;
    ind = strd.indexOf('$$&JX;2$node$&JX;$');
    if (ind >= 0) {
      sub2 = strd.substr(ind);
      strd = strd.substr(0, ind);
      sub2 = sub2.replace('$$&JX;2$node$&JX;$', '').trim();
      if (sub2 != 'undefined' && sub2 != 'null') {
        sub2 = sub2 == 'true' ? true : false;
      } else {
        sub2 = false;
      }
    }

    ex_data.setMap(process.threadId, '*' + name, embedFunc(strGlobals, strd,
        ';var _____cl=false;var continueLogic=function(){}, ____back=[];' +
        'var ___method=' + strl +
        ';exports.call = function(___cbid, ___param){if(exports.waitFirst){' +
        '____back.push( {c:___cbid, p:___param} );' +
        'if(!_____cl){_____cl=true;process.keepAlive();' +
        'continueLogic = function(){exports.waitFirst=null;process.release();' +
        'for(var o in ____back){ if (!____back.hasOwnProperty(o)) continue;' +
        'var res=exports.call(____back[o].c, ____back[o].p);' +
        'if(res!=undefined && ____back[o].c>=0){/* _$__callBack(res); */}' +
        '};____back=[];' +
        '};' +
        '}return;};' +
        'try{var ___x=___method(___param);' +
        'if(___x==undefined || ___cbid==-1){' +
        'return JSON.stringify({"_id":___cbid, dummy:true})};' +
        'return JSON.stringify({"_id":___cbid, "o":___x});' +
        '}catch(e){console.log("error during task execution:",e);' +
        ' throw "task execution: " + e;}' +
        '};', sub2));
  }
};

var cnt = 0;
var taskChecker;
var inter;
var cached = {};

var sleep = function(timeout) {
  setTimeout(function() {
    jxcore.utils.continue();
  }, timeout);
  jxcore.utils.pause();
};

process.unloadThread = function() {
  restarted = false;
  inner.setThreadOnHold(1);
  jxcore.tasks.killThread(process.threadId, process.__reset);
};

var _exception = null;
process.on('$$restart', function(err) {
  try {
    restarted = true;
    if (!process.emit('restart', process.unloadThread)) {
      process.__reset = true;
      if (err) {
        Error.captureStackTrace(err);
        console.error('restarting thread ' + process.threadId + ' due to error',
            err + '\n' + err.stack);
      }
      setTimeout(function() {
        process.unloadThread();
      }, 1);
    }
  } catch (e) {
    console.error('process.on -> restart', e);
  }
});

var runner = function(param) {
  var name = '~NodeJXThread-' + param[0];
  var thread;
  if (cached[name]) {
    thread = cached[name];
  } else {
    var source = ex_data.readMap(process.threadId, '*' + name);
    thread = require(name, 0, 0, 3, source);
    cached[name] = thread;
  }

  var w = [];
  if (typeof param[2] === 'string')
    w = JSON.parse(param[2]);

  try {
    if (!thread.call) {
      throw "did you 'return' from 'define' method? you should not!";
    }

    var x = thread.call(param[1], w);
    delete w;
    delete param[1];
    delete param[2];
    delete param;

    return x;
  } catch (e) {
    inner.setThreadOnHold(1);
    clearInterval(inter);// keep thread on hold
    _exception = e;
    process.emit('$$restart', e);
  }
};

process.on('uncaughtException', function(err) {
  if (_exception || err) {
    inner.setThreadOnHold(1);
    clearInterval(inter);// keep thread on hold
    setTimeout(function() {
      process.emit('$$restart', err);
    }, 1);
  }
});

function handleCustomMessage(msg) {
  if (msg.length) {
    for (var o = 0, ln = msg.length; o < ln; o++) {
      var m = null;

      if (msg[o] == 'null')
        continue;

      try {
        m = JSON.parse(msg[o]);
      }
      catch (e) {
        continue;
      }

      if (!m)
        continue;

      if (m.data && m.data.$runIn === 1) {
        m = m.data;
        incWaitCounter();
        func([m.id, m.mt]);
        var ret = runner([m.id, m.cbId, m.param]);

        if (m.cbId != -1 && ret) {
          decWaitCounter();
          var r = JSON.parse(ret);
          if (r.dummy) {
            delete r.dummy;
            r.o = null;
            ret = JSON.stringify(r);
          }
          r = null;
          tw.sendToAll(-1, ret, process.threadId);
        } else {
          // reduce main to previous state
          process.release();
        }
      } else {
        jxcore.tasks.emit('message', m.tid, m.data);
      }
    }
  }
}

// if mt or mt-keep, the loop interval can be slower
var taskPeek = 500;

var _timeout = setInterval;
if (MTnoKeep) {
  _timeout = setTimeout;
}

inter = _timeout(function() {
  // dummy
}, taskPeek);

function fastLoop(msg) {
  if (msg) {
    handleCustomMessage(msg);
  }

  if (!process.__reset) {
    taskChecker();
  }
}

process.on('threadMessage', fastLoop);

taskChecker = function() {
  var q = inner.compiler(process.threadId, func, runner);

  if (q == -1 || process.__reset) {
    if (q == -1) {
      process.__reset = true;
    }
    clearInterval(inter);
  }

  return q;
};
taskChecker.toString = function() {
  return null;
};

inner.setThreadOnHold(0);

if (MTnoKeep) {
  process.keepAlive();

  process.on('exit', function() {
    process.__reset = true;
    process.release();
  });
}
