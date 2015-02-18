// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');
var binding = require('./build/Release/binding');
assert.equal('world', binding.hello());
console.log('binding.hello() =', binding.hello());
