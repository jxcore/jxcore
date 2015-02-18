// Copyright & License details are available under JXCORE_LICENSE file

var fs = require('fs');
var assert = require('assert')

console.log("__filename", __filename);
console.log("__dirname", __dirname);
console.log("process.cwd()", process.cwd());

// was not able to read __filename when this module is executed as jx package
var s = fs.readFileSync(__filename);
assert.ok(s, "Cannot read __filename");