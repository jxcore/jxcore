// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

common.error('before');

// custom error throwing
throw ({ foo: 'bar' });

common.error('after');