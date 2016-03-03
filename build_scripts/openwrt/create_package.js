#!/usr/bin/env jx
// Copyright & License details are available under JXCORE_LICENSE file

if (typeof jxcore === 'undefined') {
  console.error("This script requires JXcore to run. \n" +
  "Please visit https://github.com/jxcore/jxcore-release to download JXcore for your platform");
  process.exit(1);
}

var osInfo = jxcore.utils.OSInfo();

if (!osInfo.isUbuntu) {
  console.error("You need a Ubuntu machine or VM to prepare OpenWRT package");
  process.exit(2);
}

var fs = require('fs');
var path = require('path');
var color = jxcore.utils.console;
var image_lnk = process.argv[2];


if (process.argv.length == 2) {
  color.log("usage:");
  color.log("sudo ./create_package.js configure", "green");
  color.log("./create_package.js <OpenWrt SDK URL> <mipsel or arm>", "green");
  process.exit(0);
}

function run(cmd) {
  var res = jxcore.utils.cmdSync(cmd);
  if (res.exitCode != 0) {
    color.log(res.out, "red");
    process.exit(1);
  }
  color.log("Done: [" + cmd + "]", "green");
}

if (process.getuid() == 0) {
  color.log("Do not use root user for this operation.", "red");
  process.exit(1);
}

if (process.argv < 4 || (process.argv[3] != "arm" && process.argv[3] != "mipsel")) {
  color.log("Usage: create_package <OpenWrt SDK link> <arch>", "green");
  color.log("arch: mipsel or arm", "green");
  process.exit(0);
}

jxcore.utils.cmdSync('rm -rf OpenWrt-SDK*');
color.log("Downloading SDK file", "green");
run("wget  " + image_lnk);

var files = fs.readdirSync(process.cwd());

var filename;
for (var o in files) {
  if (!files.hasOwnProperty(o)) continue;
  if (files[o].indexOf("OpenWrt-SDK") >= 0) {
    filename = files[o];
    break;
  }
}

if (!filename) {
  color.log("couldn't find OpenWrt-SDK file", "red");
  process.exit(1);
}

color.log("Unpacking OpenWRT", "green");
run("tar jxf " + filename);

files = fs.readdirSync(process.cwd());
filename = null;
for (var o in files) {
  if (!files.hasOwnProperty(o)) continue;
  if (files[o].indexOf("OpenWrt-SDK") >= 0) {
    if (fs.statSync(files[o]).isDirectory()) {
      filename = files[o];
      break;
    }
  }
}

if (!filename) {
  color.log("Error: Couldn't find OpenWrt-SDK folder", "red");
  process.exit(1);
}

jxcore.utils.cmdSync("mv " + filename + " OpenWrt-SDK");

target = path.resolve("OpenWrt-SDK/package/");

if (target[target.length - 1] != path.sep)
  target += "/";

// create jxcore folder under OpenWRT package
if (fs.existsSync(target + "jxcore")) {
  var res = jxcore.utils.cmdSync("rm -rf " + target + "jxcore");
  if (res.exitCode == -1) {
    console.error(res.out);
    process.exit(1);
  }
}

fs.mkdirSync(target + "jxcore");

color.log("Preparing Makefile", "green");
// copy Makefile
var fdata = fs.readFileSync(path.join(__dirname, "Makefile")) + "";
fdata = fdata.replace("--dest-cpu=mipsel", "--dest-cpu=" + process.argv[3]);

fs.writeFileSync(path.join(target, "jxcore/Makefile"), fdata);

color.log("Please wait, cloning JXcore into target", "green");
var cp = require('child_process');
cp.exec("git clone https://github.com/jxcore/jxcore " + target + "jxcore/jxcore", function(err, stdout, stderr) {
  if (err) {
    console.error("Failed", stdout, stderr);
    return;
  }
  color.log("Cloning sub modules", "green");
  jxcore.utils.cmdSync("cd jxcore; git submodule init; git submodule update;");

  color.log("OpenWRT package builder is ready", "green");
  color.log("Visit OpenWrt-SDK folder and run 'make V=s'", "green");
});