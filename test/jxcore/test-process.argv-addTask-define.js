// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code compares process.argv called from main thread and addTask() with define()
 */

var assert = require('assert');
var clog = jxcore.utils.console.log;
var finished = false;

var value = JSON.stringify(process.argv);

var method = function () {
  // for the console output is better to display it from a main thread
  jxcore.store.shared.set("value", JSON.stringify(process.argv));
};

process.on('exit', function (code) {
  assert.ok(finished, "The test did not finish.");
});

jxcore.tasks.addTask({define: method}, null, function (err, ret) {

  finished = true;
  var ret = jxcore.store.shared.get("value");

  clog("main thread:", "green");
  clog("\t", value);

  clog("addTask() with define():", "magenta");
  clog("\t", ret);

  assert.strictEqual(value, ret, "Values are not equal: process.argv");

  // forcing faster exit than naturally
  setTimeout(process.exit, 10);
});