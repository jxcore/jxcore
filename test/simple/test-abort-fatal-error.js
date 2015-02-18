// Copyright & License details are available under JXCORE_LICENSE file


if(process.versions.sm) { return; }

var assert = require('assert');
var common = require('../common');

if (process.platform === 'win32') {
  console.log('skipping test on windows');
  process.exit(0);
}

var exec = require('child_process').exec;

var cmdline = 'ulimit -c 0; ' + process.execPath;
cmdline += ' --max-old-space-size=1 --max-new-space-size=1';
cmdline += ' -e "setInterval(function() { new Buffer(1024); }, 1);"';

exec(cmdline, function(err, stdout, stderr) {
  assert(err);
  assert(stderr.toString().match(/abort/i));
});