// Copyright & License details are available under JXCORE_LICENSE file

if (process.IsEmbedded)
  return;

// -------------   init part

var assert = require('assert');
var path = require("path");
var fs = require("fs");


var native = process.argv[process.argv.length - 1] == "native";

var cwd = process.cwd();
var file_js = path.join(__dirname, "assets/node_path/file.js");
var file_jx = path.join(__dirname, "assets/node_path/file");
file_jx += (native ? ".exe" : ".jx");

if (fs.existsSync(file_jx))
  fs.unlinkSync(file_jx);

// creating the package
process.chdir(path.dirname(file_js));
var cmd = '"' + process.execPath + '" package file.js file';
cmd += (native ? " -native" : "");
var ret = jxcore.utils.cmdSync(cmd);
//console.log(ret);
process.chdir(cwd);

if (native)
  fs.renameSync(file_jx.replace(".exe", ''), file_jx);

if (!fs.existsSync(file_jx))
  throw "Package is not ready:\n" + file_jx;

var dir = path.dirname(file_jx);

// -------------   exec part

// folder, where the module stays
var node_path = path.join(__dirname, "assets/subfolder2");

// running the package with NODE_PATH (works also on windows)
var c = process.platform === 'win32' ? "set" : "export";
var cmd = 'cd ' + dir + ' && ' + c + ' NODE_PATH=' + node_path + ' &&';
if (native) {
  var fname = path.basename(file_jx);
  if (process.platform !== 'win32') fname = "./" + fname;
  cmd += fname;
} else {
  cmd += '"' + process.execPath + '" ' + path.basename(file_jx);
}

var ret = jxcore.utils.cmdSync(cmd);

//testing the result
var mod = require("./assets/subfolder2/node_path_module/index");
var cl = function (str) {
  return jxcore.utils.console.setColor(str, "yellow");
};

if (ret.out.indexOf(mod.value2) === -1) {
  console.error("Executing the package with NODE_PATH pointing to embedded dir failed:");
  console.log(cl('dir:') + ' `' + node_path + "`");
  console.log(cl("command used:") + " \n" + cmd);
  console.log(cl("output:"), ret.out, "\n\n");
  process.exit(-1);
}
