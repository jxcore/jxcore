// Copyright & License details are available under JXCORE_LICENSE file

/*
 This test unit calls setBlockTimeout() in main thread and checks if getBlockTimeout() returns proper value,
 also when called inside a task.
 */

var jx = require('jxtools');
var assert = jx.assert;

var value = 3456;
var shared = jxcore.store.shared;

shared.setBlockTimeout(value);

assert.strictEqual(shared.getBlockTimeout(), value, "getBlockTimeout() should return " + value);

var method = function () {
  return jxcore.store.shared.getBlockTimeout();
};

jxcore.tasks.addTask(method, null, function (ret) {
  assert.strictEqual(ret, value, "1. getBlockTimeout() should return " + value);
});


var q = {
  define: function () {
  }, logic: method
};

jxcore.tasks.addTask(q, null, function (ret) {
  assert.strictEqual(ret, value, "2. getBlockTimeout() should return " + value);
});