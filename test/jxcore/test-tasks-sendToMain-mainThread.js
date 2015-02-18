// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');

var received = false;

jxcore.tasks.on('message', function (tid, msg) {
  received = true;
});

process.on("exit", function (code) {
  assert.ok(received, "sendToMain() was not received from main thread.");
});

// testing sendToMain() from main thread
process.sendToMain("from main thread");
