// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code should be released when launched with mt-keep, but at some point was not
 */


var jx = require('jxtools');
var assert = jx.assert;

if (process.threadId !== -1)
  process.release();

var time = 3000;

setTimeout(function () {
  assert.ok(false, "The process was not released within " + time + " ms.");
}, time).unref();
