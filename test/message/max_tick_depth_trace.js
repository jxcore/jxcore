// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');

process.maxTickDepth = 10;
process.traceDeprecation = true;
var i = 20;
process.nextTick(function f() {
  console.error('tick %d', i);
  if (i-- > 0)
    process.nextTick(f);
});