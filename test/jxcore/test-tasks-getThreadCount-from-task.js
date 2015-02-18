// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit compares value of getThreadCount() from main thread and a subthread
 */


var assert = require('assert');

var cnt = 0;
var threads = 7;
jxcore.tasks.setThreadCount(threads);


jxcore.tasks.on('message', function (threadId, params) {
  cnt++;
  assert.strictEqual(params, threads, "Thread no " + threadId + " returned " + params + " from jxcore.tasks.getThreadCount() instead of " + threads);
});


jxcore.tasks.runOnce(function (obj) {
  process.sendToMain(jxcore.tasks.getThreadCount());
});


process.on("exit", function (code) {
  assert.strictEqual(cnt, threads, "Did not receive a message from some threads");
});

