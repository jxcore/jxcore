// Copyright & License details are available under JXCORE_LICENSE file

var jx = require('jxtools');
var assert = jx.assert;

var org = "<HTML></HTML>";

var s = unescape("%3CHTML%3E%3C%2FHTML%3E");
assert.strictEqual(s, org, "Unescape result for test 1 '" + s + "' is different than original '" + org + "'");

var s2 = unescape(escape(org));
assert.strictEqual(s2, org, "Unescape result for test 2 '" + s2 + "' is different than original '" + org + "'");

