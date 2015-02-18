// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing if `emptyQueue` is called when adding a task

var jx = require('jxtools');
var assert = jx.assert;

var received = false;

var complete = function () {
  assert.ok(received, "jxcore.tasks.on('emptyQueue' was not invoked");
  jx.exitNow();
};

jxcore.tasks.on("emptyQueue", function () {
  received = true;
  complete();
});

// in case if 'emptyQueue' was not called...
process.on('exit', function () {
  complete();
});

var task = function () {
  var start = Date.now();
  while (Date.now() < start + 20) {
  }
};


jxcore.tasks.addTask(task);