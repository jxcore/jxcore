// Copyright & License details are available under JXCORE_LICENSE file

// This code renders releases.json with jxcore releases basic information
// for each zip package present in jxcore/out_binaries.
// Also, if you have git release file present under out_binaries/Releases,
// e.g. jxcore-0.3.0.1.tar.gz and jxcore-0.3.0.1.zip
// then sha256 is rendered for them and added to "source" value.
// Usage:
// $ cd jxcore
// $ ./jx tools/download_packages/render_index_json.js

var version = process.jxversion.toLowerCase().replace('beta', "").replace(/[-|\.|v]/g, "").trim();
var config = require('./config.json');

var fs = require("fs");
var path = require("path");
var repoPath = path.join(__dirname, "../../");
var jxtools = require(path.join(repoPath,  "test/node_modules/jxtools"));

var out_dir = null;
var out_file = path.join(__dirname, "releases.json");

var parsedArgv = jxcore.utils.argv.parse();

if (process.argv[2]) {
  var _dir = path.resolve(process.argv[2]);
  if (fs.existsSync(_dir)) {
    out_dir = _dir;
    jxcore.utils.console.info('Rendering for', process.argv[2]);
  }
}

if (!out_dir) {
  out_dir = path.join(__dirname, "../../out_binaries");
  jxcore.utils.console.info('Rendering for', 'out_binaries');
}


// unpacks jxcore release zip file and tests `jx -p process.versions`
var getVersion = function(zipFile) {

  var tmpDir = path.join(out_dir, '_tmp');
  var tmpJX = path.join(tmpDir, path.basename(zipFile, '.zip'), 'jx');
  jxtools.rmdirSync(tmpDir);

  var ret = jxrun('unzip ' + zipFile + ' -d ' + tmpDir);
  if (!ret || !fs.existsSync(tmpJX)) {
    jxcore.utils.console.error('Could not unpack', zipFile);
    jxtools.rmdirSync(tmpDir);
    process.exit(1);
  }


  ret = jxrun(tmpJX + ' -e "console.log(JSON.stringify(process.versions))"');
  if (!ret) {
    jxcore.utils.console.error('Could not check process.versions for ', tmpJX);
    jxtools.rmdirSync(tmpDir);
    process.exit(1);
  }

  ret = JSON.parse(ret.trim());
  var ret1 = jxrun(tmpJX + ' -jsv"', true);
  if (ret1)
    ret.jsv = ret1.trim();

  jxtools.rmdirSync(tmpDir);
  return ret;
};


var jxrun = function(cmd, silent) {
  var ret = jxcore.utils.cmdSync(cmd);
  if (ret.exitCode) {
    if (!silent)
      jxcore.utils.console.error(cmd + " error:\n", ret.out);
  } else {
    return ret.out;
  }
};

var downloadFile = function(dir, url) {
  console.log('Downloading', url);
  var cmd = 'cd "' + dir + '" && wget ' + url;
  var ret = jxrun(cmd);
  if (!ret) process.exit();
  console.log('OK');
};

var get_npmjx_info = function(version) {
  var v = version.replace(/v|-|\s|beta/gi, "");
  if (!config.npmjx[v])
    return jxcore.utils.console.error('No information for npmjx["' + v + '"] found in config.json');

  var url = config.npmjx[v];
  var p = require('url').parse(url);
  var fname = path.basename(p.pathname);

  var expectedDir = path.join(__dirname, '../../out_binaries/npmjx');
  var expectedFile = path.join(expectedDir, fname);
  if (!fs.existsSync(expectedDir)) {
    var ret = jxrun('mkdir -p "' + expectedDir + '"');
    if (!ret) process.exit();
  }

  if (!fs.existsSync(expectedFile))
    downloadFile(expectedDir, url);

  if (!fs.existsSync(expectedFile))
    return jxcore.utils.console.error('Cannot find', expectedFile);

  var ret1 = jxrun("shasum " + expectedFile);
  var sha1 = ret1 ? ret1.split(" ")[0] : '';
  var ret1 = jxrun("shasum -a 256 " + expectedFile);
  var sha256 = ret1 ? ret1.split(" ")[0] : '';

  var obj = {
    "file" : fname,
    "url" : url,
    "sha256" : sha256,
    "sha1" : sha1
  }

  return obj;
};


var osinfo = jxcore.utils.OSInfo();
var engines = {
  sm: getVersion(path.join(out_dir, 'jx_' + osinfo.OS_STR + 'sm.zip')),
  v8: getVersion(path.join(out_dir, 'jx_' + osinfo.OS_STR + 'v8.zip'))
};


var manifestArr = fs.existsSync(out_file) ? require(out_file) : [];
var manifestEntry = JSON.parse(JSON.stringify(engines.sm));
manifestEntry.v8 = engines.v8.v8 + '';
manifestEntry.sm = engines.sm.sm + '';
delete manifestEntry.jsv;

var version = manifestEntry.jxcore;
var v = manifestEntry.jxcore;
// git archive cuts last ".0" in e.g. 0.3.0.0
if (v.slice(-4) === '.0.0') v = v.slice(0, -2);
v = v.replace(/v|-|\s|beta/gi, "")

manifestEntry.source = {
  zip : { url : 'https://github.com/jxcore/jxcore/archive/v' + v + '.zip' },
  tar : { url : 'https://github.com/jxcore/jxcore/archive/v' + v + '.tar.gz' }
};

var zipball = path.join(__dirname, '../../out_binaries/Releases', 'v' + v + '.zip');
if (!fs.existsSync(zipball))
  downloadFile(path.dirname(zipball), manifestEntry.source.zip.url);
if (fs.existsSync(zipball)) {
  var ret1 = jxrun("shasum -a 256 " + zipball);
  if (ret1) manifestEntry.source.zip.sha256 = ret1.split(" ")[0];
} else {
  jxcore.utils.console.error('Cannot calculate sha256 for manifestEntry.source.zip.sha256. No file:\n', zipball);
}

var tarball = path.join(__dirname, '../../out_binaries/Releases', 'v' + v + '.tar.gz');
if (!fs.existsSync(tarball))
  downloadFile(path.dirname(tarball), manifestEntry.source.tar.url);
if (fs.existsSync(tarball)) {
  var ret1 = jxrun("shasum -a 256 " + tarball);
  if (ret1) manifestEntry.source.tar.sha256 = ret1.split(" ")[0];
} else {
  jxcore.utils.console.error('Cannot calculate sha256 for manifestEntry.source.tar.sha256. No file:\n', tarball);
}

manifestEntry.npmjx = get_npmjx_info(manifestEntry.jxcore);

manifestEntry.dist = [];
for(var o in manifestArr) {
  if (manifestArr[o].jxcore === version) {
    if (!parsedArgv.force)
      return jxcore.utils.console.warn('Definitions for', v, 'are already added in', out_file);
    else
      manifestArr.splice(manifestArr.indexOf(o), 1);
  }
}

//return;
for(var o in config.builds) {
  var build = config.builds[o];

  for (var a in build.files) {

    // single file
    var file = build.files[a];
    var arch = "";
    if (file.indexOf("32") !== -1) arch = "ia32";
    if (file.indexOf("ARM") !== -1) arch = "ARM";
    if (file.indexOf("64") !== -1) arch = "x64";
    if (file.indexOf("MIPS") !== -1) arch = "MIPS";
    var engine = "";
    if (file.indexOf("v8.") !== -1) engine = "v8";
    if (file.indexOf("sm.") !== -1) engine = "sm";

    var zipFile = path.join(out_dir, file);
    if (!fs.existsSync(zipFile)) {
      jxcore.utils.console.log('No', file, 'for', version);
      continue;
    }

    var ret1 = jxrun("shasum " + zipFile);
    var sha1 = ret1 ? ret1.split(" ")[0] : '';
    var ret1 = jxrun("shasum -a 256 " + zipFile);
    var sha256 = ret1 ? ret1.split(" ")[0] : '';

    var size = fs.statSync(zipFile).size / 1024 / 1024;

    var name = build.name;
    if (build.props && build.props[file] && build.props[file].caption) name = build.props[file].caption;

    manifestEntry.dist.push({
      file : file,
      url : "https://jxcore.s3.amazonaws.com/" + (version.match(/[0-9]/g)).join('') + "/" + file,
      platform : name,
      engine : engine || undefined,
      arch : arch || undefined,
      sha256 : sha256,
      sha1 : sha1
    });
  }
}

manifestArr.push(manifestEntry);
fs.writeFileSync(out_file, JSON.stringify(manifestArr, null, 4));