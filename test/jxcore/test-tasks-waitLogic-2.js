// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing usage of waitLogic & continueLogic().
 Checks, if logic() is not executed before given time
 */

var jx = require('jxtools');
var assert = jx.assert;

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


var total = 10;
var cnt = 0;

// in case if task's callback was not called...
process.on('exit', function () {
  assert.notStrictEqual(cnt, 0, "Callback for the task was not called");
  assert.strictEqual(cnt, total, "Callback for the task was called " + cnt + " times instead of " + total);
});


for (var o = 0; o < total; o++) {
  jxcore.tasks.addTask(task, o + " Hello", function (ret) {
    var waited = ret > 450;
    assert.ok(waited, "Logic waited only " + ret + " instead of 2000 ms.");

    cnt++;
    //if (cnt === total)
    //    jx.exitNow();
  });
}