// Copyright & License details are available under JXCORE_LICENSE file

// this test checks for name conflicting, when calling require("name") - [node_modules/name]
// from inside name.js
// this occurs for packages (jx and native)

var assert = require('assert');
var util = require('util');
exports.local = true;

// requiring node module
var name = "test-require-name-conflict";
var mod = require(name);
console.log("loaded module:", util.inspect(mod, {showHidden: true, depth: 99}));

assert.ok(mod, "Cannot load node module " + name);
assert.ok(mod.test, "Loaded incorrect module");
assert.strictEqual(mod.test(), "ok", "Incorrect result from module's test() method.");


// requiring itself
var name = "./test-require-name-conflict.js";
var mod = require(name);
console.log("loaded module:", util.inspect(mod, {showHidden: true, depth: 99}));

assert.ok(mod, "Cannot load node file " + name);
assert.strictEqual(mod.test, undefined, "Loaded incorrect module");
