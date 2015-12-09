// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

if (process.argv[2] === 'child') {
  process.exit(0);
} else {
  var spawn = require('child_process').spawn;
  var child = spawn(process.execPath, [process.argv[1], 'child']);

  var error = {};
  child.on('exit', function() {
    child._handle = {
      kill: function() {
        process._errno = 42;
        return -1;
      }
    };
    child.once('error', function(err) {
      error = err;
    });
    child.kill();
  });

  process.on('exit', function() {
    // we shouldn't reset errno since it accturlly isn't set
    // because of the fake .kill method
    assert.equal(error.syscall, 'kill');
  });
}