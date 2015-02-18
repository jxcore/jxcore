// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing simple jxcore.tasks.killThread() usage.
 */

var assert = require('assert');

// main thread receives the message from a task
jxcore.tasks.on("message", function (threadId, obj) {
  if (obj.started) {
    //kill the task after a second
    setTimeout(function () {
      jxcore.tasks.killThread(threadId);
    }, 1000);
  }
});

// adding a task
jxcore.tasks.addTask(function () {

  process.keepAlive();
  process.sendToMain({started: true});

  process.on('restart', function (cb) {
    process.release();
  });

  setTimeout(function () {
    throw "Still inside the thread. The thread was not killed.";
    process.release();
  }, 3000);
});

