// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating a package with js files located in root folder and subfolder.
 Then it checks, if we can access those files.
 */


// -------------   init part

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var dir = __dirname + path.sep + path.basename(__filename) + "-tmp-dir" + path.sep;
var js_file = dir + "some_library.js";
var jx_file = dir + "my_package.jx";
var finished = false;

jx.rmdirSync(dir);
fs.mkdirSync(dir);
process.chdir(dir);

process.on("exit", function (code) {
  jx.rmdirSync(dir);
  assert.ok(finished, "Test did not finish!");
});

// -------------   exec part

var module_name = "module.js";

var subFolder_name = "folder1";
var subFolder = dir + subFolder_name + path.sep;
fs.mkdirSync(subFolder);

// writing some assets
fs.writeFileSync(dir + module_name, "exports.value = 'dir';");
fs.writeFileSync(subFolder + module_name, "exports.value = '" + subFolder_name + "';");


// contents of a js file
var arr = [
  'exports.value1 = require("./module.js").value;',
  'exports.value2 = require("./' + subFolder_name + '/module.js").value;',
  'exports.getAsset = function(path) { return require(path).source; };'
];
fs.writeFileSync(js_file, arr.join("\n"));


// we create a package first
var child = cp.exec('"' + process.execPath + '" package ' + path.basename(js_file) + " my_package", {timeout: 30000}, function (error, stdout, stderr) {

  assert.ok(fs.existsSync(jx_file), "Cannot find compiled package " + jx_file);

  // trying to access `module.js' and `folder1/module.js`
  fs.unlinkSync(dir + "module.js");
  fs.unlinkSync(subFolder + "module.js");

  var lib = require(jx_file);

  // then we check if those modules were correctly required() inside a package
  assert.strictEqual(lib.value1, "dir", "Value from module.js is:\n" + lib.value1 + "\nbut should be: `dir`");
  assert.strictEqual(lib.value2, subFolder_name, "Value from module.js is:\n" + lib.value2 + "\nbut should be: `" + subFolder_name + "`");

  finished = true;
});
