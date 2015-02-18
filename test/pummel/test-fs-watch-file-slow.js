// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

var FILENAME = path.join(common.tmpDir, 'watch-me');
var TIMEOUT = 1300;

var nevents = 0;

try {
  fs.unlinkSync(FILENAME);
}
catch (e) {
  // swallow
}

fs.watchFile(FILENAME, {interval:TIMEOUT - 250}, function(curr, prev) {
  console.log([curr, prev]);
  switch (++nevents) {
  case 1:
  case 2:
    assert.equal(fs.existsSync(FILENAME), true);
    break;
  case 3:
    assert.equal(fs.existsSync(FILENAME), false);
    fs.unwatchFile(FILENAME);
    break;
  default:
    assert(0);
  }
});

process.on('exit', function() {
  assert.equal(nevents, 3);
});

setTimeout(createFile, TIMEOUT);

function createFile() {
  fs.writeFileSync(FILENAME, "test");
  setTimeout(touchFile, TIMEOUT);
}

function touchFile() {
  fs.writeFileSync(FILENAME, "test");
  setTimeout(removeFile, TIMEOUT);
}

function removeFile() {
  fs.unlinkSync(FILENAME);
}