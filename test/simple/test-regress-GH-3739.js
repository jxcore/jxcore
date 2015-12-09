// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common.js'),
    assert = require('assert'),
    fs = require('fs'),
    path = require('path');

var dir = path.resolve(common.fixturesDir),
    dirs = [];

// Make a long path.
for (var i = 0; i < 50; i++) {
  dir = dir + '/123456790';
  try {
    fs.mkdirSync(dir, '0777');
  } catch (e) {
    if (e.code == 'EEXIST') {
      // Ignore;
    } else {
      cleanup();
      throw e;
    }
  }
  dirs.push(dir);
}

// Test existsSync
var r = fs.existsSync(dir);
if (r !== true) {
  cleanup();
  throw new Error('fs.existsSync returned false');
}

// Text exists
fs.exists(dir, function(r) {
  cleanup();
  if (r !== true) {
    throw new Error('fs.exists reported false');
  }
});

// Remove all created directories
function cleanup() {
  for (var i = dirs.length - 1; i >= 0; i--) {
    fs.rmdirSync(dirs[i]);
  }
}