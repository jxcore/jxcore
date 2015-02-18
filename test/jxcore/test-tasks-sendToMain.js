// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing process.sendToMain() from inside a task
 */

var assert = require('assert');

var tids = {};
var threads = 5;

jxcore.tasks.setThreadCount(threads);

jxcore.tasks.on('message', function (tid, msg) {
  assert.notStrictEqual(tid, -1, "tasks.on('message') passes -1 as threadId arg.");
  tids[tid] = true;
});

process.on("exit", function (code) {
  for (var a = 0; a < threads; a++) {
    assert.ok(tids[a], "sendToMain() was not received from thread " + a);
  }
});

// testing sendToMain() from task
jxcore.tasks.runOnce(function () {
  process.sendToMain("from thread");
  process.keepAlive(200);
});