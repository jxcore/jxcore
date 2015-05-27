// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');

function threadTask(g) {
  process.sendToThreads(g);
}

jxcore.tasks.setThreadCount(2);

jxcore.tasks.runOnce(function () {
  process.keepAlive();
  var aa = 0;
  jxcore.tasks.on("message", function (tid, g) {
    process.sendToMain(tid + "" + g);
    aa++;
    if (aa >= 20000)
      process.release();
  });
  process.sendToMain(1); // send once
});

var totals = 20000;
var counters = [0, 0];
var starter = 0;

jxcore.tasks.on("message", function (_, __) {
  if (starter < 2) {
    starter++;
  }

  if (starter == 2) { // make sure the all threads are live
    starter = 3;
    for (var p = 0; p < totals; p++)
      jxcore.tasks.addTask(threadTask, p);
  }
  else if (starter > 2)
    counters[_]++;
});

process.on('exit', function () {
  assert.strictEqual(counters[0] + counters[1], totals * 2, "thread messaging leak! " + counters[0] + " + " + counters[1] + " != " + (totals * 2));
});