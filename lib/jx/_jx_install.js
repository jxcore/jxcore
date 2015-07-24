// Copyright & License details are available under JXCORE_LICENSE file

exports.install = function() {
  var cc = jxcore.utils.console;
  var fs = require('fs');
  var pathModule = require('path');

  var cnf = require('_jx_config');
  var jxpath = cnf.__jx_global_path;

  var parsedArgv = jxcore.utils.argv.parse();
  var autoremove_err = false;
  var autoremove_arr = null;
  // check for --autoremove
  if (parsedArgv.autoremove) {
    autoremove_arr = parsedArgv.autoremove.splitBySep();
    if (!autoremove_arr) {
      autoremove_err = "Invalid --autoremove value: "
          + (parsedArgv.autoremove.value || "--empty--") + ".";
    }
  }

  if (process.argv.length < 2 || autoremove_err) {
    if (autoremove_err)
      cc.error(autoremove_err);
    cc.log("usage: install [package name]");
    cc.log("optional: install [package name]@[version]");
    cc.log("optional: install -g [package name]");
    cc.log("optional: install [package name] --autoremove '*.gz;samples'");
    cc.log('');
    process.exit();
    return;
  }

  // downloads the file though proxy, if --proxy or --https-proxy argv is
  // provided
  var download_through_proxy = function(url, target, cb) {
    var url_module = require('url');
    var parsed_url = url_module.parse(url);

    var proxy_url = parsed_url.protocol.toLowerCase() === "https:" ? jxcore.utils.argv
        .getValue("https-proxy")
        : jxcore.utils.argv.getValue("proxy");
    if (!proxy_url)
      return false;

    var parsed_proxy_url = url_module.parse(proxy_url);

    var http = require('http');
    var https = require('https');

    // tunnel options
    var opts = {
      host : parsed_proxy_url.hostname,
      port : parsed_proxy_url.port,
      method : 'CONNECT',
      path : parsed_url.hostname,
      headers : {
        Host : parsed_url.hostname
      }
    };

    if (parsed_proxy_url.auth)
      opts.headers["Proxy-Authorization"] = 'Basic '
          + new Buffer(parsed_proxy_url.auth).toString('base64');

    http.request(opts).on('connect', function(res, socket, head) {
      https.get({
        host : parsed_url.hostname,
        path : parsed_url.path,
        socket : socket,
        agent : false
      }, function(res) {
        var file = fs.createWriteStream(target);
        res.on('data', function(chunk) {
          file.write(chunk);
        }).on('end', function() {
          file.end();
        });
        file.on('finish', function() {
          file.close();
          setTimeout(cb, 1000);
        })
      });
    }).on('error', function(e) {
      console.error(e);
      process.exit(1);
    }).end();

    return true;
  };

  // we need to remove it before passing argv to npm
  jxcore.utils.argv.remove("--autoremove", true);
  var args = process.argv;

  var download = function(url, target, cb) {
    if (download_through_proxy(url, target, cb))
      return;

    var http = require('https');
    var req = http.request(url, function(res) {
      var file = fs.createWriteStream(target);
      res.on('data', function(chunk) {
        file.write(chunk);
      }).on('end', function() {
        file.end();
      });
      file.on('finish', function() {
        file.close();
        setTimeout(cb, 1000);
      })
    });

    req.end();
  };
  
  var name = "";
  var npm_basename = "npmjxv1_7.jx";
  var npm_str = "https://s3.amazonaws.com/nodejx/" + npm_basename;
  var isWindows = process.platform === 'win32';
  var homeFolder = process.__npmjxpath || process.env.HOME
      || process.env.HOMEPATH || process.env.USERPROFILE;
  var jxFolder = homeFolder + pathModule.sep + ".jx";
  var targetBin = jxFolder + pathModule.sep + "npm";
  var npmrcPath = targetBin + pathModule.sep + "npmrc";

  function Install() {
    var arr = [];
    if (name.trim() == "-global") {
      name = "-g";
    }
    if (name.indexOf("-") === 0 && name.indexOf("--") < 0
        && name.trim() != "-g") {
      args[2] = args[2].substr(1);
      arr.push(targetBin);
      cmd = true;
    } else if (args[1] === "npm") {
      arr.push(targetBin);
      cmd = true;
    } else {
      if (name.trim() == "-g" && jxpath) {
        args[args.length] = "--prefix=" + jxpath;
      }
      arr.push(targetBin);
      arr.push("install");
    }

    arr = arr.concat(args.slice(2));
    var found = false;

    // copying npm settings, if available
    if (pathModule.basename(targetBin) === "npm") {
      var ret = jxcore.utils.cmdSync("npm config ls -l");
      if (!ret.exitCode && ret.out) {
        fs.writeFileSync(npmrcPath, ret.out);
        found = true;
      }
    }

    if (!found) {
      arr.push("--loglevel");
      arr.push("http");
      if (!isWindows) {
        // https://docs.npmjs.com/misc/config#color
        // Default: true on Posix, false on Windows
        arr.push("--color");
        arr.push("true");
      }
    }

    // spawn allows to pass formatted output (colors)
    var child = require('child_process').spawn(process.execPath, arr, {
      stdio : "inherit"
    });
    if (autoremove_arr) {
      child.on("close", function(code) {

        var modules = parsedArgv["_"].withoutPrefix || [];
        for (var a = 0, len = modules.length; a < len; a++) {
          var _module = modules[a];

          var npm_cmd = process.execPath + " npm ls --depth=0 -s ";
          if (parsedArgv.g || parsedArgv.global)
            npm_cmd += "-g ";
          // determine the folder, where module was installed
          var ret = jxcore.utils.cmdSync(npm_cmd + _module);
          if (ret.exitCode) {
            cc.warn("Cannot determine path of installed module:", _module);
            continue;
          }
          var _arr = ret.out.trim().split("\n");
          var _path = _arr[0];
          // vertical bar - not a pipe sign
          var verticalBar = String.fromCharCode.apply(null, [ 9474 ]);
          if (_path.slice(0, 1) === verticalBar)
            _path = _path.slice(1).trim();

          var _path = pathModule.join(_path, "node_modules", _module);
          if (!fs.existsSync(_path)) {
            cc
                .warn("Cannot find expected folder of installed module:",
                    _module);
            continue;
          }

          // makes decision, whether remove file/folder or not
          var checkRemove = function(folder, file, path, isDir) {

            var specials = [ "\\", "^", "$", ".", "|", "+", "(", ")", "[", "]",
                "{", "}" ]; // without '*' and '?'

            for ( var o in autoremove_arr) {
              if (!autoremove_arr.hasOwnProperty(o))
                continue;

              var mask = autoremove_arr[o];
              var isPath = mask.indexOf(pathModule.sep) !== -1;

              // entire file/folder basename compare
              if (mask === file)
                return true;

              // compare against entire path (without process.cwd)
              if (isPath
                  && path.replace(process.cwd(), "").indexOf(mask) !== -1)
                return true;

              // regexp check against * and ?
              var r = mask;
              for ( var i in specials) {
                if (specials.hasOwnProperty(i))
                  r = r.replace(new RegExp("\\" + specials[i], "g"), "\\"
                      + specials[i]);
              }

              var r = r.replace(/\*/g, '.*').replace(/\?/g, '.{1,1}');
              var rg1 = new RegExp('^' + r + '$');
              var rg2 = new RegExp('^' + pathModule.join(folder, r) + '$');
              if (rg1.test(file) || rg2.test(path))
                return true;
            }

            return false;
          };

          // removes files/folders defined with --autoremove
          delTree(_path, checkRemove);
        }
      });
    }
  }

  var delTree = function(loc, checkRemove) {
    if (fs.existsSync(loc)) {
      var _files = fs.readdirSync(loc);
      var _removed = 0;
      for ( var o in _files) {
        if (!_files.hasOwnProperty(o))
          continue;

        var file = _files[o];
        var _path = loc + pathModule.sep + file;
        if (!fs.lstatSync(_path).isDirectory()) {
          try {
            var removeFile = checkRemove
                && checkRemove(loc, file, _path, false);
            if (!checkRemove || removeFile) {
              fs.unlinkSync(_path);
              _removed++;
              if (removeFile)
                cc.log("--autoremove:", _path.replace(process.cwd(), '.'),
                    "yellow");
            }
          } catch (e) {
            cc.write("Permission denied ", "red");
            cc.write(loc, "yellow");
            cc.log(" (do you have a write access to this location?)");
          }
          continue;
        }
        // folders
        var removeDir = checkRemove && checkRemove(loc, file, _path, true);
        if (!checkRemove || removeDir) {
          delTree(_path);
          if (removeDir)
            cc
                .log("--autoremove:", _path.replace(process.cwd(), '.'),
                    "yellow");
        } else {
          delTree(_path, checkRemove);
        }
      }

      if (!checkRemove || _removed == _files.length)
        fs.rmdirSync(loc);
    }
  };

  function tryNPM() {
    var forced = false;
    if (!fs.existsSync(jxFolder + pathModule.sep + process.jxversion)) {
      forced = true;
    }

    if (!forced && fs.existsSync(jxFolder + pathModule.sep + "npm")) {
      Install();
      return;
    }

    cc.log("Preparing NPM for JXcore (" + process.jxversion
        + ") for the first run", "yellow");

    if (!fs.existsSync(jxFolder)) {
      var ec = jxcore.utils.cmdSync("mkdir  " + (isWindows ? "" : "-p ")
          + jxFolder);
      if (ec.exitCode != 0 || !fs.existsSync(jxFolder)) {
        console
            .error("Make sure the user account has a write permission to it's home folder. >> "
                + ec.out
                + "\n Consider using a custom path from jx.config file's 'npmjxPath' property.");
        try {
          process.exit(1);
        } catch (e) {
        }
      }
    }

    if (forced) {
      delTree(jxFolder + pathModule.sep + "npm");
      fs.writeFileSync(jxFolder + pathModule.sep + process.jxversion, "1");
    }

    targetBin = jxFolder + pathModule.sep + npm_basename;
    download(npm_str, targetBin, function() {
      Install();
    });
  }

  if (args.length > 2)
    name = args[2];

  tryNPM();
};
