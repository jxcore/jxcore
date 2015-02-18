// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing usage of waitLogic & continueLogic().
 Checks, if logic() is not executed before given time
 */

var jx = require('jxtools');
var assert = jx.assert;

var callbackCalled = false;

var task = {
  define: function () {
    var start = Date.now();
    setTimeout(function () {
      continueLogic();
    }, 500);
  },
  logic: function (obj) {
    return Date.now() - start;
  },
  waitLogic: true
};

// in case if task's callback was not called...
process.on('exit', function () {
  assert.ok(callbackCalled, "Callback for the task was not called");
});


jxcore.tasks.addTask(task, " Hello", function (ret) {
  callbackCalled = true;
  var waited = ret > 450;
  assert.ok(waited, "Logic waited only " + ret + " instead of 2000 ms.");
  jx.exitNow();
});
