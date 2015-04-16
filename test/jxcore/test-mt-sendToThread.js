// Copyright & License details are available under JXCORE_LICENSE file

/*
 In this test unit (executed with mt-keep) we are sending from each thread
 `count` messages to one, chosen thread (receiverThread)
 */

var jx = require('jxtools');
var assert = jx.assert;

var threads = parseInt(process.argv[1].replace("mt-keep:", ""));

if (isNaN(threads)) {
  threads = 2; // default value for thread pool
}

var receiverThread = threads - 1;

var count = 50;
var counter = 0;
var cnts = {};

var done = function() {
  for (var a = 0; a < threads; a++) {
    assert.strictEqual(cnts[a], count, "Thread " + process.threadId + " received from thread " + a + " " + cnts[a] + " messages instead of " + count);
  }
  process.exit();
};

jxcore.tasks.on('message', function (threadId, msg) {
  counter++;
  if (!cnts[msg.x]) {
    cnts[msg.x] = 1;
  } else {
    cnts[msg.x]++;
  }

  if (process.threadId == receiverThread && counter === count * threads)
    done();
});

setTimeout(function () {
  var tm = count;
  while (tm--) {
    process.sendToThread(receiverThread, {x: process.threadId});
  }

  if (process.threadId == receiverThread) {
    // if done() was not called already...
    setTimeout(done, 10000);
  } else {
    setTimeout(process.release, 700);
  }

}, 700);


