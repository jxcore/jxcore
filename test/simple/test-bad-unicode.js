// Copyright & License details are available under JXCORE_LICENSE file


var assert = require('assert'),
    exception = null;

try {
  eval('"\\uc/ef"');
} catch (e) {
  exception = e;
}

assert(exception instanceof SyntaxError);