// Copyright & License details are available under JXCORE_LICENSE file

// npm for jx
var http = require('https')
  , path = require('path')
  , fs = require("fs");

if (process.argv[1].toLowerCase().lastIndexOf('npmjxv3.jx') != process.argv[1].length - 10) {
  console.error("usage: jx install [package_name]");
  console.error(">>", process.argv.join(" "));
  process.exit(1);
}

var getOptions = function (name) {
  for (var o = 0; o < process.argv.length; o++) {
    if (process.argv[o] == name) {
      if (process.argv.length > o + 1)
        return process.argv[o + 1];
      else
        return true;
    }
  }
  return false;
};

// downloads the file though proxy, if --proxy or --https-proxy argv is provided
var download_through_proxy = function (url, target, cb) {

  var url_module = require('url');
  var parsed_url = url_module.parse(url);

  var proxy_url = parsed_url.protocol.toLowerCase() === "https:" ? getOptions("--https-proxy") : getOptions("--proxy");
  if (!proxy_url)
    return false;

  var parsed_proxy_url = url_module.parse(proxy_url);

  var http = require('http');
  var https = require('https');

  // tunnel options
  var opts = {
    host: parsed_proxy_url.hostname,
    port: parsed_proxy_url.port,
    method: 'CONNECT',
    path: parsed_url.hostname,
    headers : {
      Host: parsed_url.hostname
    }
  };

  if (parsed_proxy_url.auth)
    opts.headers["Proxy-Authorization"] = 'Basic ' + new Buffer(parsed_proxy_url.auth).toString('base64');

  http.request(opts).on('connect', function(res, socket, head) {
    https.get({
      host: parsed_url.hostname,
      path : parsed_url.path,
      socket: socket,
      agent: false
    }, function(res) {
      var file = fs.createWriteStream(target);
      res.on('data', function (chunk) {
        file.write(chunk);
      }).on('end', function () {
        file.end();
      });
      file.on('finish', function () {
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

var download = function (url, target, cb) {

  if (download_through_proxy(url, target, cb))
    return;

  var counter = 0;

  var req = http.request(url, function (res) {
    var file = fs.createWriteStream(target);
    res.on('data', function (chunk) {
      file.write(chunk);
      counter += chunk.length;
      if (counter >= 1e6) {
        counter = 0;
        process.stdout.write(".");
      }
    }).on('end', function () {
      file.end();
      process.stdout.write(".");
    });
    file.on('finish', function () {
      console.log("");
      file.close();
      setTimeout(cb, 1000);
    })
  });

  req.end();
};

var npmloc = __dirname + path.sep + "npm";
var exec = require('child_process').exec;

function clear_files(folder) {
  try {
    var files = fs.readdirSync(folder);

    for (var o in files) {
      var name = files[o];

      if (name.indexOf("._") == 0) {
        fs.unlink(folder + path.sep + name);
      } else {
        var stat = fs.statSync(folder + path.sep + name);
        if (stat.isDirectory()) {
          clear_files(folder + path.sep + name);
        }
      }
    }
  } catch (e) {
    return;
  }
}

var extract = function (loc, target, cb, targz) {
  new targz().extract(loc, target, function (err) {
    if (err)
      cb(0, err);
    else {
      var clear = false;
      try {
        if (fs.existsSync(__dirname + path.sep + "npmjxv3.jx")) {
          clear = true;
          fs.unlink(__dirname + path.sep + "npmjxv3.jx");
        }
      } catch (e) {
      }
      if (clear) {
        clear_files(target);
        cb(1, "NPM for JX is ready.");
      }
      else {
        cb(1, "JXcore embedded is downloaded");
      }
    }
  });
};

var gonpm = function () {
  if (process.argv[0].indexOf('"') < 0)
    process.argv[0] = '"' + process.argv[0] + '"';

  if (process.argv[1].indexOf('"') < 0)
    process.argv[1] = '"' + npmloc + '"';

  console.log("executing... please wait.");

  var ec = exec(process.argv.join(" "), {maxBuffer: 1e8},
    function (error, stdout, stderr) {
    });
  ec.stdout.pipe(process.stdout);
  ec.stderr.pipe(process.stderr);
};

jxcore.utils.console.log("Downloading NPM for JXcore", "yellow");

download("https://s3.amazonaws.com/nodejx/npmjx.tar.gz", npmloc + ".tar.gz", function () {
  var targz = require('tar.gz');
  extract(npmloc + ".tar.gz", __dirname, function (isdone, msg) {
    console.log(msg);
    if (!isdone) {
      process.exit(1);
      return;
    }

    gonpm();
  }, targz);
});



