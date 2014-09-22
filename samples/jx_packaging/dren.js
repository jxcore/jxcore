/** 
  Copyright Nubisa, Inc. (2014) (MIT LICENSE)
  
  Alters the 'node.exe' inside the binary to the application's file name. Since for most native binaries 'node.exe' is
  an expected host, it only updates from 'node.exe'
  
  usage:
  
  jx dren.js TARGET_.NODE_FILE NEW_EXE_NAME
  
  sample:
  
  jx dren.js "node_modules\\fibers\\bin\\win32_ia32_v8_3.14\\fibers.node" "test.exe"

  OR YOU CAN REPLACE ALL THE .NODE FILES UNDER NODE_MODULES
  
  jx dren.js * "test.exe"

  "*" means target all the .node files under node_modules folder
  
  remarks:
  
  Once you alter the node.exe to test.exe, you may not replace it back. (better install the module again to change the name)
  Target executable name can not be bigger than 8 chars length. (including .exe part)
  This solution provides a hack! Better test your application after altering the name. The target file can be corrupted! 
  
**/
if(typeof jxcore == 'undefined'){
  console.error('This application requires jxcore to run.');
  process.exit(-1);
}

var fs = require('fs');
var pathModule = require('path');
var log = jxcore.utils.console.log;
var args = process.argv;
var check = function(c, m){if(!c){log(m, "red");process.exit(-1);}};

check(process.platform == 'win32', "This application is Windows only, you don't need this functionality on other systems");
check(args.length==4, "Requires a path for the .node file and a new target");
check(args[3].length <= 8, "Requires the new target name length max 8 chars");


var new_name = args[3];
var nchar = "n".charCodeAt(0), ochar = "o".charCodeAt(0);
if(new_name.length<8){
  var diff = 8 - new_name.length;
  new_name = new_name + new Array(diff).join(" ");
}
new_name = new Buffer(new_name);

function replace(file_name){
  var fsize = fs.statSync(file_name).size, loc = 1024; 
  var fd = fs.openSync(file_name, 'r+');

  while(loc<fsize){
    var buffer = new Buffer(1024);
    fs.readSync(fd, buffer, 0, 1024, loc);
    var skip = -1;
    for(var i=0, ln = buffer.length;i<ln;i++){
      if(buffer[i] == nchar){
	    var pass = true;
	    if(i<ln-1){
	      if(buffer[i+1] != ochar){
		    pass = false;
		  }
        }
	    if(pass){
	      skip = i;
	      break;
	    }
	  }
    }
    if(skip == -1){
      loc += 1024;
	  continue;
    }
    loc += skip;
    var sub_buffer = new Buffer(8);
    fs.readSync(fd, sub_buffer, 0, 8, loc);
    if(sub_buffer+"" == "node.exe"){
      fs.writeSync(fd, new_name, 0, 8, loc);
	  loc+=8;
    }
    else{
      loc++;
    }
  }
  fs.closeSync(fd);
  log("Completed", file_name, "green");
}

function search(target){
  var dirs = fs.readdirSync(target);
  for(var o in dirs){
    var dir_name = target + pathModule.sep + dirs[o];
	  var stat = fs.statSync(dir_name);
	  if(stat.isDirectory()){
	    search(dir_name);
	  }
	  else if(pathModule.extname(dir_name) == ".node"){
	    replace(dir_name);
	  }
  }
}

if(args[2] != "*"){
  check(fs.existsSync(args[2]), "Could not find the given .node file");
  replace(args[2]);
}else{
  var nm_dir = process.cwd() + "\\node_modules";
  check(fs.existsSync(nm_dir), "Could not find node_modules folder");
  search(nm_dir);
}
