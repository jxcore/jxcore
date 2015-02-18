// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing proper behaviour of setThreadCount / getThreadCount
 and also, if defined number of threads corresponds to actual thread count.
 */


var assert = require('assert');

var x = jxcore.tasks.getThreadCount();
assert.strictEqual(x, 2, "Wrong number of threads. Should be 2 (default value for JXcore), but is: " + x);

var number = 7;
jxcore.tasks.setThreadCount(number);
var x = jxcore.tasks.getThreadCount();

assert.strictEqual(x, number, "Wrong number of threads. Should be " + number + " and is: " + x);


