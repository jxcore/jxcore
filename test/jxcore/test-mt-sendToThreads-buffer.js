// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code is testing sending unicode strings with process.sendToThreads()
 */

var jx = require('jxtools');
var assert = jx.assert;

//process.keepAlive();

var buf = new Buffer(20);
buf.fill(20);
buf.write("some string", 0);
var str = buf.toString();
var finished = false;

process.on("exit", function (code) {
  assert.ok(finished, "Test unit did not finish.");
});

jxcore.tasks.on('message', function (threadId, params) {
  assert.strictEqual(str, params, "sentToThreads(): strings not equal: " + str + " !== " + params);
  if (process.threadId !== -1)
    process.release();
  finished = true;
});


// sending buffer as string, but some of bytes are uninitialized
// when running with mt, each thread sends to all other threads
process.sendToThreads(str);
