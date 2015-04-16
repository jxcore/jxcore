// Copyright & License details are available under JXCORE_LICENSE file

/*
 In this test unit (executed with mt-keep) we are sending from each thread
 `count` messages to all other threads and we check if all of messages has arrived
 */

var jx = require('jxtools');
var assert = jx.assert;

var threads = parseInt(process.argv[1].replace("mt-keep:", ""));

if (isNaN(threads)) {
  threads = 2; // default value for thread pool
}

var count = 50;
var counter = 0;
var cnts = {};

var done = function() {
  for (var a = 0; a < threads; a++) {
    assert.strictEqual(cnts[a], count, "Thread " + process.threadId + " received from thread " + a + " " + cnts[a] + " messages instead of " + count);
  }
  if (process.subThread)
    process.release();
};

jxcore.tasks.on('message', function (threadId, msg) {
  counter++;
  if (!cnts[msg.x]) {
    cnts[msg.x] = 1;
  } else {
    cnts[msg.x]++;
  }

  if (counter === count * threads)
    done();
});


setTimeout(function () {
  var tm = count;
  while (tm--) {
    process.sendToThreads({x: process.threadId});
  }
  // if done() was not called already...
  setTimeout(done, 10000).unref();
}, 700);


