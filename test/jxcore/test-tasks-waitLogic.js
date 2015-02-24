// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing usage of waitLogic & continueLogic().
 Checks, if logic() is not executed before given time
 */

var jx = require('jxtools');
var assert = jx.assert;

var task = {
  define: function () {
    setTimeout(function () {
      continueLogic();
    }, 500);
  },
  logic: function (obj) {
    // do nothing
  },
  waitLogic: true
};

var start = Date.now();

process.on('exit', function () {
  var now = Date.now() - start;
  assert.ok(now > 450, "Logic waited only " + now + " instead of 2000 ms.");
});

jxcore.tasks.addTask(task, " Hello");
