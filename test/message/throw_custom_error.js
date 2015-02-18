// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

common.error('before');

// custom error throwing
throw ({ name: 'MyCustomError', message: 'This is a custom message' });

common.error('after');