// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing  process.subThread and process.threadId inside main thread, task, and when executed with mt
 */

var method = function (sid) {
  var assert = require('assert');

  sid += ", threadId: " + process.threadId + ". ";
  assert.strictEqual(process.subThread, true, sid + "process.subThread is " + process.subThread + " but should be true.");
  assert.notStrictEqual(process.threadId, -1, sid + "process.subThread is -1 but should be" + process.threadId);
};


// task
jxcore.tasks.runOnce(method, "runOnce()");
