// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');

var finished = 0;
var cnt = 5;

process.on('exit', function (code) {
  assert.strictEqual(finished, cnt, "Only " + finished + " tasks finished instead of " + cnt);
});


var method = function (ms) {
  process.keepAlive();

  var start = Date.now();
  do {
    // let's make the thread busy for some time
  }
  while (Date.now() - start < ms);
  if (process.threadId !== -1)
    process.release();
};

assert.strictEqual(jxcore.tasks.jobCount(), 0, "Before adding task1: tasks.jobCount should be 0.");

jxcore.tasks.addTask(method, 100, function () {
  setTimeout(function () {
    assert.strictEqual(jxcore.tasks.jobCount(), 0, "After executing task1: tasks.jobCount should be 0.");

    for (var a = 0; a < cnt; a++) {
      jxcore.tasks.addTask(method, '1000', function () {
        finished++;
        if (finished == cnt) {
          setTimeout(function () {
            assert.strictEqual(jxcore.tasks.jobCount(), 0, "After executing all tasks: tasks.jobCount should be 0.");
          }, 5);
        }
      });
    }
    // immediately, after adding cnt tasks, we check jobCount()
    // since single task lasts for 1000 ms, right here jobCount() should be = cnt
    assert.strictEqual(jxcore.tasks.jobCount(), cnt, "After adding " + cnt + " tasks: tasks.jobCount is " + jxcore.tasks.jobCount() + ", but should be " + cnt);

  }, 5);

});

