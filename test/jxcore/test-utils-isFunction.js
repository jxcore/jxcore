// Copyright & License details are available under JXCORE_LICENSE file

// This unit tests jxcore.utils.isFunction()


var assert = require('assert');

var f = function (cb) {
  return false;
};

var obj = {
  m: function () {
  }
};

// object
var fn = new function () {
  this.fn = function () {
  };
  this.fn1 = function (a) {
  };
  this.fn2 = new function (a) {
  }; // <- object
};


var nonFunctions = [1, "2", true, false, "", "string", {
  s: function () {
  }
}, -2, [], [function () {
}, ""], obj, fn, fn.fn2];
var functions = [function () {
}, function (s) {
}, f, obj.m, fn.fn, fn.fn1];

for (var o in nonFunctions) {
  if (nonFunctions.hasOwnProperty(o)) {
    var is = jxcore.utils.isFunction(nonFunctions[o]);
    assert.strictEqual(is, false, "This is not a function, but isFunction() returned true! item id = " + o + ": " + nonFunctions[o]);
  }
}

for (var o in functions) {
  if (functions.hasOwnProperty(o)) {
    var is = jxcore.utils.isFunction(functions[o]);
    assert.strictEqual(is, true, "This is a function, but isFunction() returned false! item id = " + o + ": " + functions[o]);
  }
}
