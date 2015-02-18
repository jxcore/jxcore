// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

// üäö

console.log('Σὲ γνωρίζω ἀπὸ τὴν κόψη');

assert.equal(true, /Hellö Wörld/.test('Hellö Wörld'));