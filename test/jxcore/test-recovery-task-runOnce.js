// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing thread recovery created with tasks
 */


var assert = require('assert');

var method = function () {

  process.keepAlive();

  var firstRun = jxcore.store.shared.setIfNotExists("restart_" + process.threadId, true);

  if (firstRun) {
    process.on('restart', function (cb) {
      jxcore.store.shared.set("onRestart" + process.threadId, true);
      cb();
    });

    setTimeout(function () {
      throw "Expected throw!";
    }, 1000);
  }
  else {
    jxcore.store.shared.set("RESTARTED" + process.threadId, true);
  }
};

var threads = 3;

process.on('exit', function (code) {
  var cnt = 0;
  for (var a = 0; a < threads; a++) {
    var flag1 = jxcore.store.shared.read("onRestart" + a);
    assert.equal(flag1, "true", "Thread no " + a + " did not receive on('restart').");

    var flag2 = jxcore.store.shared.read("RESTARTED" + a);
    assert.equal(flag2, "true", "Thread no " + a + " did not restart.");
  }
});


jxcore.tasks.setThreadCount(threads);
jxcore.tasks.runOnce(method);

// just for case if process will not exit by itself?
setTimeout(process.exit, 10000).unref();