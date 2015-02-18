// Copyright & License details are available under JXCORE_LICENSE file

var assert = require('assert');
var index = require("mymodule");

var str = "value exported from index.js";
assert.strictEqual(index.value1, str, "Wrong value was read. This: `" + index.value1 + "` instead of this: `" + str + "`");
console.log(JSON.stringify(index, null, 4));