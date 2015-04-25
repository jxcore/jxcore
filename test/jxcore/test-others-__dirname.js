// Copyright & License details are available under JXCORE_LICENSE file

var fs = require('fs');
var path = require('path');
var assert = require('assert')

console.log("Dump:");
console.log("\t__filename", __filename);
console.log("\t__dirname", __dirname);
console.log("\tprocess.cwd()", process.cwd());
console.log("\tpath.dirname(process.execPath)", path.dirname(process.execPath));

if (process.isPackaged)
  assert.strictEqual(__dirname, path.dirname(process.execPath), "For native packages, __dirname is not equal to path.dirname(process.execPath)");
else
  assert.strictEqual(__dirname, path.dirname(__filename), "__dirname is not equal to path.dirname(process.execPath)");
