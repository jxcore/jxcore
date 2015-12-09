// Copyright & License details are available under JXCORE_LICENSE file

var common = require('../common');
var assert = require('assert');
var util = require('util');

// test the custom implementation on SM
if (!process.versions.sm) return;

var counter = 0;
Error.prepareStackTrace = function(a,b) {
  counter++;
};

try {
  throw "";
} catch(e) {
  Error.captureStackTrace(e);
}

try {
  throw undefined;
} catch(e) {
  Error.captureStackTrace(e);
}

try {
  throw new Error("");
} catch(e) {
  Error.captureStackTrace(e);
}

assert.equal(counter, 1)
