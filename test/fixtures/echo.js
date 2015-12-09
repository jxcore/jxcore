// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

common.print('hello world\r\n');

var stdin = process.openStdin();

stdin.on('data', function(data) {
  process.stdout.write(data.toString());
});