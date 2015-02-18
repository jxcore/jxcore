// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');


var ran = false;
var stop = false;
var start = +new Date();

function spin() {
  var now = +new Date();
  if (now - start > 200) { // Increased from 100 for DEBUG & (interpreter only testing)
    throw new Error('The timer is starving');
  }

  if (!stop) {
    ran = true;
    process.nextTick(spin);
  }
}

function onTimeout() {
  stop = true;
}

spin();
setTimeout(onTimeout, 50);

process.on('exit', function() {
  assert.ok(ran);
  assert.ok(stop);
});