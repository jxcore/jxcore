// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var path = require('path');
var match = false;

var isDebug = process.features.debug;

var debugPaths = [path.normalize(path.join(__dirname, '..', '..',
                                           'out', 'Debug', 'jx')),
                  path.normalize(path.join(__dirname, '..', '..',
                                           'Debug', 'node'))];
var defaultPaths = [path.normalize(path.join(__dirname, '..', '..',
                                             'out', 'Release', 'jx')),
                    path.normalize(path.join(__dirname, '..', '..',
                                             'Release', 'jx'))];

console.error('debugPaths: ' + debugPaths);
console.error('defaultPaths: ' + defaultPaths);
console.error('process.execPath: ' + process.execPath);

if (isDebug) {
  debugPaths.forEach(function(path) {
    match = match || process.execPath.indexOf(path) == 0;
  });
} else {
  defaultPaths.forEach(function(path) {
    match = match || process.execPath.indexOf(path) == 0;
  });
}

assert.ok(match);