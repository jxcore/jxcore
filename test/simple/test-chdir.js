// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

assert.equal(true, process.cwd() !== __dirname);

process.chdir(__dirname);
assert.equal(true, process.cwd() === __dirname);

var dir = path.resolve(common.fixturesDir,
    'weird \ud83d\udc04 characters \ud83d\udc05');
fs.mkdirSync(dir);
process.chdir(dir);
assert(process.cwd() == dir);

process.chdir('..');
assert(process.cwd() == path.resolve(common.fixturesDir));
fs.rmdirSync(dir);