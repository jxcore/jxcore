// Copyright & License details are available under JXCORE_LICENSE file

var common = require('../common');
var assert = require('assert');
var util = require('util');

// test the custom implementation on SM
// See https://github.com/jxcore/jxcore/issues/728
if (!process.versions.sm) return;

function getCallerFile() {
  var oldPrepareStackTrace = Error.prepareStackTrace;
  Error.prepareStackTrace = function(err, stack) { stack.dd = 3; return stack; };
  var stack = new Error().stack;
  Error.prepareStackTrace = oldPrepareStackTrace;

  return stack[2] ? stack[2].getFileName() : undefined;
}

assert.equal(getCallerFile(), "module.js")
