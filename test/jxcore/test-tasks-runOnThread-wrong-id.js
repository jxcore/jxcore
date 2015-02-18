// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing tasks.runOnThread() for invalid threadId.
 */

var assert = require('assert');

var cnt = 5;
var finished = 0;

jxcore.tasks.setThreadCount(cnt);

process.on('exit', function () {
  assert.ok(finished, "Test did not finish!");
});

function method() {
  return true;
}

// we expect an exception
try {
  jxcore.tasks.runOnThread(7, method, null, function (ret) {
    throw "This method should not be executed.";
  });
} catch (ex) {
  finished = true;
}





