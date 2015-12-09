// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var exec = require('child_process').exec;
var path = require('path');

var callbacks = 0;

function test(env, cb) {
  var filename = path.join(common.fixturesDir, 'test-fs-readfile-error.js');
  var execPath = process.execPath + ' --throw-deprecation ' + filename;
  var options = { env: env || {} };
  exec(execPath, options, function(err, stdout, stderr) {
    assert(err);
    assert.equal(stdout, '');
    assert.notEqual(stderr, '');
    cb('' + stderr);
  });
}

test({ NODE_DEBUG: '' }, function(data) {
  assert(/EISDIR/.test(data));
  assert(!/test-fs-readfile-error/.test(data));
  callbacks++;
});

test({ NODE_DEBUG: 'fs' }, function(data) {
  assert(/EISDIR/.test(data));
  assert(/test-fs-readfile-error/.test(data));
  callbacks++;
});

process.on('exit', function() {
  assert.equal(callbacks, 2);
});

(function() {
  console.error('the warnings are normal here.');
  // just make sure that this doesn't crash the process.
  var fs = require('fs');
  fs.readFile(__dirname);
  fs.readdir(__filename);
  fs.unlink('gee-i-sure-hope-this-file-isnt-important-or-existing');
})();