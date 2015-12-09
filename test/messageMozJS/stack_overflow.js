// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

common.error('before');

// stack overflow
function stackOverflow() {
  stackOverflow();
}
stackOverflow();

common.error('after');