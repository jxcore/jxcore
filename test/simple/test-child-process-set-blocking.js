// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var ch = require('child_process');

var SIZE = 100000;
var childGone = false;

var cp = ch.spawn('python', ['-c', 'print ' + SIZE + ' * "C"'], {
  customFds: [0, 1, 2]
});

cp.on('exit', function(code) {
  childGone = true;
  assert.equal(0, code);
});

process.on('exit', function() {
  assert.ok(childGone);
});