// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing  process.subThread and process.threadId inside main thread and when executed with mt
 */

var jx = require('jxtools');
var assert = jx.assert;

if (process.IsEmbedded || process.argv[1].slice(0, 2) !== "mt") {
  // main thread
  assert.strictEqual(process.subThread, false, "main thread: process.subThread is " + process.subThread + " but should be false.");
  assert.strictEqual(process.threadId, -1, "main thread: process.subThread is " + process.threadId + " but should be -1.");
} else {
  // mt / mt-keep
  var sid = process.argv[1] + ", threadId: " + process.threadId + ". ";
  assert.strictEqual(process.subThread, true, sid + "process.subThread is " + process.subThread + " but should be true.");
  assert.notStrictEqual(process.threadId, -1, sid + "process.subThread is -1 but should be" + process.threadId);

  process.release();
}


