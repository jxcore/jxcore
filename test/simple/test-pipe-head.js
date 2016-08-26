// Copyright & License details are available under JXCORE_LICENSE file

if (process.platform === 'win32') {
  console.error('Skipping: platform is Windows.');
  process.exit(0);
}

var common = require('../common');
var assert = require('assert');

var exec = require('child_process').exec;
var join = require('path').join;

var nodePath = process.argv[0];
var script = join(common.fixturesDir, 'print-10-lines.js');

var cmd = '"' + nodePath + '" "' + script + '" | head -2';

var finished = false;

exec(cmd, function(err, stdout, stderr) {
  if (err) throw err;
  var lines = stdout.split('\n');
  assert.equal(3, lines.length);
  finished = true;
});


process.on('exit', function() {
  assert.ok(finished);
});
