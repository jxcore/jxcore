// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');

assert.strictEqual(process.mainModule.filename, __filename,
  "process.mainModule.filename & __filename should be equal, as stated in the docs.\n" +
  "process.mainModule.filename\n\t" + process.mainModule.filename + "\n" +
  "__filename:\n\t" + __filename);
