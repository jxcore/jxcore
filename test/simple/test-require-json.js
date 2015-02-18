// Copyright & License details are available under JXCORE_LICENSE file


var assert = require('assert');

try {
  require('../fixtures/invalid.json');
} catch (err) {
  var re = /test[\/\\]fixtures[\/\\]invalid.json/;
  var i = err.message.match(re);
  assert(null !== i, 'require() json error should include path');
}