// Copyright & License details are available under JXCORE_LICENSE file



var jx = require('jxtools');
var assert = jx.assert;

var finished = false;
var start = Date.now();

setTimeout(function () {
  finished = true;
}, 1000 + (process.threadId * 1000));


process.on('exit', function () {
  assert.ok(finished, "Test did not finish for thread " + process.threadId);
});
