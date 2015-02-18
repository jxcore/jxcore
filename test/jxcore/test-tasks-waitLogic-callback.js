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
    setTimeout(function () {
      continueLogic();
    }, 500);
  },
  logic: function (obj) {
  },
  waitLogic: true
};

process.on('exit', function () {
  assert.ok(callbackCalled, "Callback for the task was not called");
});


jxcore.tasks.addTask(task, null, function (ret) {
  callbackCalled = true;
});
