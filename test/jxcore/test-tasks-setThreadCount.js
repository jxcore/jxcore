// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing proper behaviour of setThreadCount / getThreadCount
 and also, if defined number of threads corresponds to actual thread count.
 */

var assert = require('assert');

var number = 7;

jxcore.tasks.setThreadCount(3);
jxcore.tasks.setThreadCount(5);

jxcore.tasks.setThreadCount(number);

var x = jxcore.tasks.getThreadCount();

assert.strictEqual(x, number, "Wrong number of threads. Should be " + number + " and is: " + x);


jxcore.tasks.on('message', function (threadId, params) {
  number--;
});


process.on("exit", function (code) {
  assert.strictEqual(number, 0, "Did not receive a message from some threads");
});


jxcore.tasks.runOnce(function (obj) {
  process.sendToMain("done");
  process.keepAlive(250);
});

