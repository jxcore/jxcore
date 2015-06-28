#!/usr/bin/env jx

if (typeof jxcore === 'undefined') {
  console.error("This script requires JXcore to run. " +
    "Please visit http://jxcore.com/downloads to download JXcore for your platform");
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
var target = process.argv[2];

if (target)
  target = path.join(target, 'package/');

if (process.argv < 4 || !target || !fs.existsSync(target) 
            || (process.argv[3] != "arm" && process.argv[3] != "mipsel")) {
  color.log("Usage: create_package <OpenWRT Image Builder Path> <arch>", "green");
  color.log("arch: mipsel or arm", "green");
  process.exit(0);
}

// create jxcore folder under OpenWRT package
if ( fs.existsSync(target + "jxcore") ) {
  var res = jxcore.utils.cmdSync("rm -rf " + target + "jxcore");
  if (res.exitCode == -1) {
    console.error(res.out);
    process.exit(1);
  }
}
fs.mkdirSync(target + "jxcore");

// copy Makefile
var fdata = fs.readFileSync(path.join(__dirname, "Makefile")) + "";
fdata = fdata.replace("--dest-cpu=mipsel", "--dest-cpu=" + process.argv[3]);

fs.writeFileSync(path.join(target, "jxcore/Makefile"), fdata);

color.log("please wait, cloning jxcore into target", "green");
var res = jxcore.utils.cmdSync("git clone https://github.com/jxcore/jxcore " + target + "jxcore/jxcore");

if (res.exitCode != 0) 
  console.error("Failed", res.out);

color.log("OpenWRT package builder is ready.\nVisit " + process.argv[2] + " and run 'make V=s'", "green");