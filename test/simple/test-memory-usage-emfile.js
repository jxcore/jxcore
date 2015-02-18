// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

var fs = require('fs');

var files = [];

while (files.length < 206)
  files.push(fs.openSync(__filename, 'r'));

var r = process.memoryUsage();
console.log(common.inspect(r));
assert.equal(true, r['rss'] > 0);