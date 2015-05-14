// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing NODE_PATH environment variable applied to
// virtual path inside package / native package

if (process.isPackaged || exports.$JXP)
  return;

// -------------   init part

var assert = require('assert');
var path = require("path");
var fs = require("fs");

var native = process.argv[process.argv.length - 1] == "native";
var ext = "";
if (!native)
  ext = ".jx";
else
  ext = process.platform === "win32" ? ".exe" : "";

var dir = path.join(__dirname, "assets/node_path");
var file_jx_basename = "file" + ext;
var file_jx = path.join(dir, file_jx_basename);
var file_jxp = path.join(dir, "file.jxp");
var file_win_bat = path.join(dir, "test_win.bat");

process.on('exit', function () {
  if (fs.existsSync(file_jx)) fs.unlinkSync(file_jx);
  if (fs.existsSync(file_jxp)) fs.unlinkSync(file_jxp);
  if (fs.existsSync(file_win_bat)) fs.unlinkSync(file_win_bat);
});

if (fs.existsSync(file_jx))
  fs.unlinkSync(file_jx);

// creating the package
process.chdir(dir);
var cmd = '"' + process.execPath + '" package file.js -add' + (native ? " -native" : "");
var ret = jxcore.utils.cmdSync(cmd);

if (!fs.existsSync(file_jx)) {
  console.error("Package is not ready:\n" + file_jx);
  console.error(ret.out);
  process.exit(1);
}

// -------------   exec part

// folder, where the module stays
var node_path = path.join(__dirname, "assets/subfolder2");

// running the package with NODE_PATH (works also on windows)
if (process.platform === 'win32') {
  var arr = [];
  arr.push("set NODE_PATH=" + node_path);
  if (native)
    arr.push(file_jx_basename);  // file
  else
    arr.push('"' + process.execPath + '" ' + file_jx_basename);  // jx file.jx

  // on windows we have to run bat file, because "set NODE_PATH... && jx file.jx"
  // does not work properly when called in one-liner
  fs.writeFileSync(file_win_bat, arr.join("\n"));
  var cmd = file_win_bat;
} else {
  var cmd = "export NODE_PATH=" + node_path + " && ";
  if (native)
    cmd += "./" + file_jx_basename;  // ./file
  else
    cmd += '"' + process.execPath + '" ' + file_jx_basename;  // jx file.jx
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
  process.exit(1);
}
