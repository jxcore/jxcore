// Copyright & License details are available under JXCORE_LICENSE file

if (process.platform === 'win32') {
  console.error('Skipping: platform is Windows.');
  process.exit(0);
}

if (process.versions.sm) {
    console.error('Skipping: engine is SpiderMonkey.');
    process.exit(0);
}

if (process.versions.v8 && parseFloat(process.versions.v8) > 3.15) {
    console.error('Skipping: engine V8 > 3.15.');
    process.exit(0);
}

var assert = require('assert');
var common = require('../common');

var exec = require('child_process').exec;

var cmdline = 'ulimit -c 0; ' + process.execPath;
cmdline += ' --max-old-space-size=1 --max-new-space-size=1';
cmdline += ' -e "setInterval(function() { new Buffer(1024); }, 1);"';

exec(cmdline, function(err, stdout, stderr) {
  assert(err);

  // works for all platforms, also for android
  assert(stderr.toString().match(/process out of memory/i));

  if (process.platform !== "android")
    assert(stderr.toString().match(/abort/i));
});