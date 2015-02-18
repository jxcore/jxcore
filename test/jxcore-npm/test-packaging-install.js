// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit calls jx install command in order to download some known packages from JXcore repo,
 and if cannot find it there - downloads by using nmp
 */

// -------------   init part

var jx = require('jxtools');
var assert = jx.assert;

var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var moduleName = process.argv[2];
if (!moduleName)
  throw "Expected module name in argv";

var finished = false;
var isError = false;

var dir = path.join(__dirname, path.basename(__filename) + "-tmp-dir");
var dir_modules = path.join(dir, "node_modules");

jx.rmdirSync(dir);
fs.mkdirSync(dir);
fs.mkdirSync(dir_modules);
process.chdir(dir_modules);

process.on("exit", function (code) {
  jx.rmdirSync(dir);
  assert.ok(finished, "Test did not finish! Exits with code " + code);
});


// -------------   exec part

var batch = path.join(dir, "test-packaging-install.bat");

fs.writeFileSync(batch, '"' + process.execPath + '" install ' + moduleName);
fs.chmodSync(batch, "755");

var child = cp.exec(batch, null, function (error, stdout, stderr) {

  var jx_path = path.join(dir, moduleName + ".jx");
  var npm_path = path.join(dir_modules, moduleName);
  var jx_ok = fs.existsSync(jx_path);
  var npm_ok = fs.existsSync(npm_path);

  var str_path = jx_ok ? jx_path : npm_path;

  if (jx_ok || npm_ok) {
    try {
      console.log("require() for " + str_path.replace(process.cwd() + path.sep, ""));
      require(str_path);
    } catch (ex) {
      throw "Error while require('" + str_path + "'):\n" + ex;
    }

  } else {
    jxcore.utils.console.log("stdout: ", "magenta");
    console.log(stdout);
    jxcore.utils.console.log("stderr: ", "magenta");
    console.log(stderr);
    throw "Could not find installed package: " + moduleName;
  }

  finished = true;
});


child.on("error", function (err) {
  throw "Cannot spawn a child: " + err;
});

