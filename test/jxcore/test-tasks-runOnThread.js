// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing tasks.runOnThread() and verifies if method was executed on the right thread.
 */

var assert = require('assert');

var cnt = 5;
var finished = 0;

jxcore.tasks.setThreadCount(cnt);

function method(id) {
  return {id: id, tid: process.threadId};
}

for (var a = 0; a < cnt; a++) {
  jxcore.tasks.runOnThread(a, method, a, function (ret) {
    assert.strictEqual(ret.id, ret.tid, "The task no " + ret.id + " was executed on wrong thread: " + ret.tid);
    finished++;
  });
}


process.on('exit', function () {
  assert.strictEqual(finished, cnt, "Test did not finish!");
});