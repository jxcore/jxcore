// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

common.error('before');

(function () {
	// these lines should contain tab!
	throw ({ foo: 'bar' });
})();

common.error('after');