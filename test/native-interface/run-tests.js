#!/usr/local/bin/jx

var fs = require('fs');
var path = require('path');

var color_log = jxcore.utils.console.log;
var args = process.argv.slice(1);

if (args.length<4) {
  color_log("usage: run-tests <libs folder> <sm or v8> <number of runs>", "green");
  process.exit(1);
}

if (!fs.existsSync(args[1] + '/include/node/public/jx.h')) {
  color_log("wrong <libs folder> path. It doesn't have 'bin/jx.h' ->", args[1], "red");
  process.exit(1);
}

if (args[2] != "v8" && args[2] != "sm") {
  color_log("second argument needs to be either 'sm' or 'v8'", "red");
  process.exit(1);
}

var runs = 1;

try{
  if (args[3])
    runs = parseInt(args[3]);
}catch(e){}

if (isNaN(runs)) {
  color_log("third argument needs to be a number", "red");
  process.exit(1);
}

var home = process.cwd();
var dirs = fs.readdirSync(home);

for(var o in dirs) {
  if (dirs[o] == 'commons') continue;
  var stat = fs.statSync(path.join(home, dirs[o]));
  if (!stat.isDirectory())
    continue;
  jxcore.utils.console.write(dirs[o] + ": ", "green");
  var ret = jxcore.utils.cmdSync("./test-single.sh " + args[1] + " " + dirs[o] + " " + args[2] + " 1");
  if(ret.exitCode != 0 || ret.out.length>200) {
    console.error(ret.out);
    process.exit(1);
  }
  
  for(var i=0; i<runs; i++) {
    ret = jxcore.utils.cmdSync("./" + dirs[o] + "/test");
    if(ret.exitCode != 0) {
      color_log("\nFailed at ", (i+1) + ".", " run:", "red");
      color_log(ret.out);
      process.exit(1);
    } else {
      jxcore.utils.console.write(".");
    }
  }
  jxcore.utils.cmdSync("rm ./" + dirs[o] + "/test");
  jxcore.utils.console.write(" (pass)\n", "green");
}