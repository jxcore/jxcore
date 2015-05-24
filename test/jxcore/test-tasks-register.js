// Copyright & License details are available under JXCORE_LICENSE file


var assert = require('assert');

var methods = function () {
  global.cwdToLower = function () {
    return process.cwd().toLowerCase();
  };

  global.myVar = "something";
};

var task = function () {

  var ret = {};
  try {
    ret.cwdToLower = cwdToLower();
    ret.myVar = myVar;
  } catch (ex) {
  }

  return ret;
};

jxcore.tasks.register(methods);

jxcore.tasks.addTask(task, null, function (err, ret) {
  methods();
  var main_ret = task();
  for (var o in main_ret)
    if (main_ret.hasOwnProperty(o))
      assert.strictEqual(main_ret[o], ret[o], "Values are not equal: main_ret[" + o + "] !== ret[" + o + "]");
});