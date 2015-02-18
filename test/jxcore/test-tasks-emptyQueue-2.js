// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing if `emptyQueue` is called when adding multiple tasks

var jx = require('jxtools');
var assert = jx.assert;

var received = false;
var callbackCalled = false;
var total = 20;
var cnt = 0;

var complete = function () {
  assert.ok(received, "jxcore.tasks.on('emptyQueue' was not invoked");
  assert.ok(callbackCalled, "Callback for the task was not called");
  assert.strictEqual(cnt, total, "emptyQueue was invoked before " + total + " tasks were finished (only " + cnt + ")");
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


for (var a = 0; a < total; a++) {
  jxcore.tasks.addTask(task, null, function () {
    callbackCalled = true;
    cnt++;
  });
}