// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

common.error('before');

var Script = process.binding('evals').NodeScript;

// undefined reference
var script = new Script('foo.bar = 5;');
script.runInNewContext();

common.error('after');