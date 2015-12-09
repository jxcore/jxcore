// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

process.nextTick(function() {
  process.nextTick(function() {
    process.nextTick(function() {
      process.nextTick(function() {
        undefined_reference_error_maker;
      });
    });
  });
});