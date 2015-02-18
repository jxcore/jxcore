// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit compares value of getThreadCount() with value from  mt/mt-keep command
 */

var jx = require('jxtools');
var assert = jx.assert;

var threads = parseInt(process.argv[1].replace("mt-keep:", "").replace("mt:", ""));
var count = jxcore.tasks.getThreadCount();

assert.ok(threads, "This test case is designed to run with mt:x or mt-keep:x");
assert.strictEqual(threads, count, "App called with " + process.argv[1] + " returns jxcore.tasks.getThreadCount() = " + count);


if (process.threadId !== -1)
  process.release();