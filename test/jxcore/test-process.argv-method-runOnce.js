// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code compares process.argv called from main thread and method.runOnce() as method
 */

var assert = require('assert');
var clog = jxcore.utils.console.log;

var value = JSON.stringify(process.argv);
var cnt = jxcore.tasks.getThreadCount();

var method = function (obj) {
  // for the console output is better to display it from a main thread
  process.sendToMain(JSON.stringify(process.argv));
};

jxcore.tasks.on('message', function (tid, ret) {
  cnt--;

  if (value !== ret) {

    clog("main thread:", "green");
    clog("\t", value);

    clog("method.runOnce() in thread", tid, "magenta");
    clog("\t", ret);

    assert.strictEqual(value, ret, "Values (main thread vs thread " + process.threadId + ") are not equal: process.argv");
  }

  if (cnt == 0) {
    // forcing faster exit than naturally
    setTimeout(process.exit, 10);
  }
});

process.on('exit', function (code) {
  assert.strictEqual(cnt, 0, "Task did not finish.");
});


method.runOnce();