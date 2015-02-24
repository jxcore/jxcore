// Copyright & License details are available under JXCORE_LICENSE file

var uw = process.binding("thread_wrap");
var jxutil = process.binding("jxutils_wrap");
var _util = require('util');

var osInfo = null;

exports.smartRequire = function(req){
	function _smartRequire(_req){
		this.req = _req;
		var _this = this;
		this.require = function(a,b,c,d){
			try{
				return _this.req(a,b,c,d);
			}catch(e){ 
				if(!(/[\/\\.]/.test(a))){
					exports.console.log("Trying NPM for", a, "yellow");
					var str = process.argv[0] + " install " + a;
					
					if(process.IsEmbedded){
						str = "jx install " + a;
					}
					
					var ret = exports.cmdSync(str);
					if(ret.exitCode == 0){
						exports.console.log("[OK] Installed", a, "green");	
					}
					else if(process.IsEmbedded){
						exports.console.log("[FAILED] Installing", a, "red")
						exports.console.log("When embedded, this feature expects JXcore available via 'jx' on the system path", "yellow");
					}
				}
				return _this.req(a,b,c,d);
			}
		};
	}
	return new _smartRequire(req).require;
};

exports.OSInfo = function(){
    if(osInfo)
      return osInfo;

    var info = exports.getOS().toLowerCase();

    osInfo = {};
    osInfo.fullName = info;
    osInfo.is64 = /x64/.test(info);
    osInfo.is32 = !osInfo.is64;
    osInfo.isARM = /arm/.test(info);
    
    if(/mobile/.test(info)) {
      osInfo.isMobile = true;
      osInfo.isIOS = /ios/.test(info);
      osInfo.isAndroid = /android/.test(info);
      osInfo.isWinRT = /winrt/.test(info);
    } else {
      osInfo.isUbuntu = /ubuntu/.test(info);
      if(!osInfo.isUbuntu){
        osInfo.isDebian = /debian/.test(info) || /raspberrypi/.test(info);
      }
      else
        osInfo.isDebian = false;
      
      osInfo.isMobile = false;
      osInfo.isMac = /osx/.test(info);
      osInfo.isRH = /red hat/.test(info);
      osInfo.isSuse = /suse/.test(info);
      osInfo.isBSD = /bsd/.test(info);
      osInfo.isArch = /arch/.test(info);
      osInfo.isWindows = /win/.test(info);
      osInfo.isGentoo = /gentoo/.test(info);
      osInfo.isLinux = /linux/.test(info);    

      if(osInfo.isUbuntu)
          osInfo.OS_STR = "ub";
      else if(osInfo.isDebian)
          osInfo.OS_STR = "deb";
      else if(osInfo.isMac)
          osInfo.OS_STR = "osx";
      else if(osInfo.isRH)
          osInfo.OS_STR = "rh";
      else if(osInfo.isSuse)
          osInfo.OS_STR = "suse";
      else if(osInfo.isSuse)
          osInfo.OS_STR = "bsd";
      else if(osInfo.isWindows)
      	osInfo.OS_STR = "win";
      else if(osInfo.isArch)
      	osInfo.OS_STR = "ark";
      else if(osInfo.isGentoo) 
        osInfo.OS_STR = "gen";
      else if(osInfo.isLinux)
        osInfo.OS_STR = "linux";
      else
        throw new Error("This operating system is not supported ("+info+")");
    }

    if(osInfo.isARM) {
      osInfo.OS_STR += "ARM";
      if (osInfo.is64) osInfo.OS_STR += "64";
    } else {
      osInfo.OS_STR += (osInfo.is64) ? "64":"32";
    }
    return osInfo;
};

exports.pause = function(check_interval){
  if(check_interval < 0)
  {
    throw new Error("jxcore.utils.pause interval can not be smaller than 0");
  }
  jxutil.runLoop(check_interval || 1);
};

exports.jump = function(){
  if(!jxutil.runLoop(-1)){
	  console.error("jump interrupted. Make sure the thread wasn't paused already");
  }
};

exports.continue = function(){
	jxutil.runLoop();
};

var lastCall = null, checkInter = null;
var _getCPU = function(cb, timer){
	if(!timer){
		var q = Date.now();
		if(lastCall){
			timer = q - lastCall;
		}
		else
			timer = 1000;
		lastCall = q;
	}
	
	if(!cb){
		throw new Error("getCPU expects (function, int)")
	}
	
    var loop = 250;
    jxutil.getCPU(Date.now(), loop);
    var left = 0;
    var cpu_usage = 0;

    setTimeout(function(){
        jxutil.getCPU(Date.now(), loop);

        var cpu_inter = setInterval(function(){
            left += loop;
            cpu_usage += jxutil.getCPU(Date.now(), loop);
            if(left>=timer){
                clearInterval(cpu_inter);
                cb( ( cpu_usage / (timer/loop) ), timer);
            }
        }, loop);
    },loop);
};

exports.getCPU = function(cb, timer){
	if(checkInter && cb){
		throw new Error("You can not run multiple instances of getCPU. Call getCPU() without parameters to shutdown the previous one");
	}
	
	if(checkInter){
		clearInterval(checkInter);
		return;
	}
	
	var haveIt = false;
	checkInter = setInterval(function(){
		if(!haveIt){
			haveIt = true;
			_getCPU(function(usage, tm){
				haveIt = false;
				cb(usage, tm);
			}, timer);
		}
	}, timer);
};

exports.cmdSync = function(command){
	if(!command)
	{
		throw 'execSync command can not be ' + (command);
	}
	
	var arr = jxutil.execSync(command + ' 2<&1');
	var res = {out:arr[0], exitCode:arr[1]};
	return res;
};

exports.getOS = function(){
	if(process.platform == "darwin" || process.platform == "win32"){
		if(process.platform == "darwin")
			return "OSX-" + process.arch;
		return "WIN-" + process.arch;
	} else if(process.platform == 'ios' || process.platform == 'android' || process.platform == 'winrt') {
	  return process.platform + "-" + process.arch + " (mobile)";
	} else{
		var fs = require('fs');
		if(fs.existsSync('/proc/version')){
			var file = fs.readFileSync("/proc/version") + "";
			var loc = file.indexOf("Red Hat");
			if(loc<0){
				loc = file.indexOf("Ubuntu");
			}
			if(loc<0){
				loc = file.indexOf("SUSE");
			}
			if(loc<0){
				loc = file.toLowerCase().indexOf("debian");
			}
			if(loc>0){
				var loc2 = file.indexOf(")", loc);
				if (loc2<0)
					loc2 = file.indexOf(" ", loc);
				file = file.substr(loc, loc2-loc);
				return file + "-" + process.arch;
			}
			
			var res = exports.cmdSync('uname -msrn');
			if(res.exitCode === 0)
				return res.out;
			return file;
		}
		else{
			return process.platform+ "-" + process.arch;
		}
	}
};

var mults = [1, 255, 255*255, 255*255*255, 255*255*255*255, 255*255*255*255*255, 255*255*255*255*255*255, 255*255*255*255*255*255*255];

exports.bufferID = function(tag){
	if(!tag || !tag.length){
		return 0;
	}
	
    var val = 0;
    var n = tag.length, i=0;
    while(n--){
        var z = tag[n];
        if(z>0){
            val += mults[i++] * z;
        }
        else{
            i++;
        }
    }

    return val;
};

exports.uniqueId = function(){
	return jxutil.getUnique();
};

exports.isFunction = function(method){
	return Object.prototype.toString.call(method) == '[object Function]';
};

// -------------- ANSI-COLOR

var ANSI_CODES = {
		'bold': ['\u001b[1m', '\u001b[22m'],
		'italic': ['\u001b[3m', '\u001b[23m'],
		'underline': ['\u001b[4m', '\u001b[24m'],
		'inverse': ['\u001b[7m', '\u001b[27m'],
		'black': ['\u001b[30m', '\u001b[39m'],
		'red': ['\u001b[31m', '\u001b[39m'],
		'green': ['\u001b[32m', '\u001b[39m'],
		'yellow': ['\u001b[33m', '\u001b[39m'],
		'blue': ['\u001b[34m', '\u001b[39m'],
		'magenta': ['\u001b[35m', '\u001b[39m'],
		'cyan': ['\u001b[36m', '\u001b[39m'],
		'white': ['\u001b[37m', '\u001b[39m'],
		'default': ['\u001b[39m', '\u001b[39m'],
		'grey': ['\u001b[90m', '\u001b[39m'],
		'bgBlack': ['\u001b[40m', '\u001b[49m'],
		'bgRed': ['\u001b[41m', '\u001b[49m'],
		'bgGreen': ['\u001b[42m', '\u001b[49m'],
		'bgYellow': ['\u001b[43m', '\u001b[49m'],
		'bgBlue': ['\u001b[44m', '\u001b[49m'],
		'bgMagenta': ['\u001b[45m', '\u001b[49m'],
		'bgCyan': ['\u001b[46m', '\u001b[49m'],
		'bgWhite': ['\u001b[47m', '\u001b[49m'],
		'bgDefault': ['\u001b[49m', '\u001b[49m']
};

function setColor(str,color,arr) {
  if(!color) return str;
  
  var color_attrs = color.split("+");
  var ansi_str = "";
  var found = null;

  for(var i=0, attr; attr = color_attrs[i]; i++) {
	if(ANSI_CODES[attr]){
		found = attr;
		ansi_str += ANSI_CODES[attr][0];
		break;
	}
  }
  
  if(found){
	  if(process.env.NOCOLOR || process.env.NODE_DISABLE_COLORS){
		  str = _util.format.apply(this, arr);
		  return str;
	  }
	  else
		  ansi_str += str + ANSI_CODES[found][1];
  }
  else{
	  arr.push(color);
	  str = _util.format.apply(this, arr);
	  ansi_str = str;
  }
  return ansi_str;
}

exports.console = {};
exports.console.setColor = setColor;

function logMessage() {
  if(arguments.length>1){
	  var arr = [];
	  for(var o=0, ln=arguments.length-1;o<ln;o++){
		  arr.push(arguments[o]+"");
	  }
	  var jump = false;
	  if(!arguments[arguments.length-1].split){
		  arr.push(arguments[arguments.length-1]);
		  jump = true;
	  }
	  var str = _util.format.apply(this, arr);
	  console.log(setColor(str, jump?null:arguments[arr.length], arr));
  }
  else{
	  console.log(arguments[0]);
  }
};

function writeMessage() {
  if(arguments.length>1){
	  var arr = [];
	  for(var o=0, ln=arguments.length-1;o<ln;o++){
		  arr.push(arguments[o]+"");
	  }
	  var str = _util.format.apply(this, arr);
	  process.stdout.write(setColor(str, arguments[arr.length], arr));
  }
  else{
	  process.stdout.write(arguments[0]);
  }
};
exports.console.log = logMessage;
exports.console.write = writeMessage;
