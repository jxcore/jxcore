// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

// This is not a great test. It depends on a Node internal, namely the slab
// size. Maybe we should expose that in some way. Then again, maybe not...
for (var n = 1; n <= 8192; ++n) {
  Buffer(n);
  Buffer(0).write('', 'base64');
}