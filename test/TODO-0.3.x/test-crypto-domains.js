// Copyright & License details are available under JXCORE_LICENSE file


var crypto = require('crypto');
var domain = require('domain');
var assert = require('assert');
var d = domain.create();
var expect = ['pbkdf2', 'randomBytes', 'pseudoRandomBytes']

d.on('error', function (e) {
  var idx = expect.indexOf(e.message);
  assert.notEqual(idx, -1, 'we should have error: ' + e.message);
  expect.splice(idx, 1);
});

d.run(function () {
  crypto.pbkdf2('a', 'b', 1, 8, function () {
    throw new Error('pbkdf2');
  });

  crypto.randomBytes(4, function () {
    throw new Error('randomBytes');
  });

  crypto.pseudoRandomBytes(4, function () {
    throw new Error('pseudoRandomBytes');
  });
});

process.on('exit', function () {
  assert.strictEqual(expect.length, 0, 'we should have seen all error messages');
});
