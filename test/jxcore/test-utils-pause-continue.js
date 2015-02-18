// Copyright & License details are available under JXCORE_LICENSE file

// this unit is testing jxcore.utils.pause() and jxcore.utils.continue()


var jx = require('jxtools');
var assert = jx.assert;


var sleep = function (timeout) {
  setTimeout(function () {
    jxcore.utils.continue();
  }, timeout);
  jxcore.utils.pause();
};


var ms = parseInt(Math.random() * 150);

var start = Date.now();
sleep(ms);
var end = Date.now();
var diff = end - start;

if (diff < ms - 10 && diff > ms + 1000) {
  assert(0, "jxcore.utils.pause() waited for " + diff + " instead of " + ms + " in thread = " + process.threadId);
}

if (process.threadId === -1) {
  var task = function () {
    require(__filename);
  };

  jxcore.tasks.addTask(task, null, function () {
    jx.exitNow();
  });
}