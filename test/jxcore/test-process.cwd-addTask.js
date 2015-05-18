// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code compares process.cwd() called from main thread and addTask() as method
 */

var assert = require('assert');
var clog = jxcore.utils.console.log;
var finished = false;

var value = JSON.stringify(process.cwd());

var method = function (obj) {
  return JSON.stringify(process.cwd());
};

process.on('exit', function (code) {
  assert.ok(finished, "The test did not finish.");
});

jxcore.tasks.addTask(method, null, function (err, ret) {

  finished = true;

  clog("main thread:", "green");
  clog("\t", value);

  clog("addTask() as method:", "magenta");
  clog("\t", ret);

  assert.strictEqual(value, ret, "Values are not equal: process.cwd()");

  // forcing faster exit than naturally
  setTimeout(process.exit, 10);
});