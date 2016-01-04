// Copyright & License details are available under JXCORE_LICENSE file

var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

var fixture = path.join(__dirname, '../fixtures/x.txt');

if (process.versions.ch)
  assert.equal('xyz\r\n', fs.readFileSync(fixture));
else if (process.platform === 'win32')
  assert.equal('xyz\r', fs.readFileSync(fixture));
else
  assert.equal('xyz\n', fs.readFileSync(fixture));