// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code compares process.mainModule.filename called from main thread and addTask() with logic()
 */

var assert = require('assert');
var clog = jxcore.utils.console.log;
var finished = false;

var value = JSON.stringify(process.mainModule.filename);

var method = function (obj) {
  try {
    return JSON.stringify(process.mainModule.filename);
  } catch(e) {
    console.log('error!', e);
    return e;
  }
};

process.on('exit', function (code) {
  assert.ok(finished, "The test did not finish.");
});

var task = {
  define: function () {},
  logic: method
};

jxcore.tasks.addTask(task, null, function (err, ret) {

  finished = true;

  clog("main thread:", "green");
  clog("\t", value);

  clog("addTask() with logic():", "magenta");
  clog("\t", ret);

  assert.strictEqual(value, ret, "Values are not equal: process.mainModule.filename");

  // forcing faster exit than naturally
  setTimeout(process.exit, 10);
});