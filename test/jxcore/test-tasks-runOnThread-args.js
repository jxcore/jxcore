// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing tasks.runOnThread() and verifies if method was executed.
 */

var assert = require('assert');
var util = require("util");

var cnt = 5;
var finished = 0;

jxcore.tasks.setThreadCount(cnt);

function method(arg) {
  return arg;
}

var args = [1, "2", "three", {four: 1}, [5], ["six"]];

for (var a = 0; a < cnt; a++) {
  jxcore.tasks.runOnThread(a, method, args[a], function (err, ret) {
    finished++;
  });
}

process.on('exit', function () {
  assert.strictEqual(finished, cnt, "Test did not finish! Some of the tasks were not executed.");
});
