// Copyright & License details are available under JXCORE_LICENSE file


var jx = require('jxtools');
var assert = jx.assert;
var crypto = require('crypto');
var fs = require('fs');
var path = require('path');

var shasum = crypto.createHash('sha1');

var s = fs.ReadStream(path.join(__dirname, "testcfg.py"));
s.on('data', function (d) {
  shasum.update(d);
});

s.on('end', function () {
  var d = shasum.digest('hex');
  assert.ok(d, "Hashing failed: " + d);

  if (process.threadId !== -1)
    process.release();

  jx.exitNowMT();
});



