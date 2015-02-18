// Copyright & License details are available under JXCORE_LICENSE file

// this test checks for name conflicting, when calling require("name") - [node_modules/name]
// from inside name.js
// this occurs for packages (jx and native)

exports.local = true;

var name = "test-require-name-conflict";
var mod = require(name);
console.log("loaded module:", require('util').inspect(mod, {showHidden: true, depth: 99}));


var assert = require('assert');
assert.ok(mod, "Cannot load node module " + name);
assert.ok(!mod.local, "Loaded __filename instead of module `" + name + "`");
assert.ok(mod.test, "Loaded incorrect module");
assert.strictEqual(mod.test(), "ok", "Incorrect result from module's test() method.");
