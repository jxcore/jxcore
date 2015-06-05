// Copyright & License details are available under JXCORE_LICENSE file

var fs = require('fs');
var path = require("path");

var base = __dirname;
var jx = require("jxtools");

var tmpBase = path.join(__dirname, "tmp_jx");
if (!fs.existsSync(tmpBase)) fs.mkdirSync(tmpBase);

var cwd = process.cwd();
var json = null;

/**
 * Displays console output
 *
 * @type {boolean}
 */
exports.silent = false;

/**
 * Forces repackaging for the same process.versions
 *
 * @type {boolean}
 */
exports.force_refresh = false;

/**
 * Enables/disables testing feature for preventing repacking jx/native packages with every consequent test run
 * @type {boolean}
 */
exports.jx_json_enabled = false;

/**
 * Compiles single js file into jx or native package and puts into output folder
 *
 * @param src -
 *          javascript filename
 * @param native -
 *          bool. true for native package (-native), false for jx package
 * @example src = /.../test/jxcore/test-http-create.js
 */
exports.createSinglePackage = function (src, native) {

  var json_file = path.join(path.dirname(src), "jx.json");
  var json = {};
  if (fs.existsSync(json_file)) {
    try {
      if (exports.jx_json_enabled)
        json = JSON.parse(fs.readFileSync(json_file).toString());
      else
        fs.unlinkSync(json_file);
    } catch (ex) {
    }
  }

  var versions = JSON.parse(JSON.stringify(process.versions));
  versions.jxv = process.jxversion;

  var suffix = native ? "native" : "package";

  var fname = path.basename(src); // eg. test-http-create.js
  var basename = path.basename(fname, ".js"); // eg. test-http-create
  var dirname = path.dirname(src); // eg. /.../test/jxcore/
  var tmpDir = path.join(tmpBase, basename + "_" + suffix);

  jx.rmdirSync(tmpDir);
  fs.mkdirSync(tmpDir);

  var compiled_binary = path.join(tmpDir, basename);
  if (!native)
    compiled_binary += ".jx";
  else if (process.platform === "win32") compiled_binary += ".exe";

  var outputDir = getOutputDir(src, native);
  var outputFile = path.join(outputDir, path.basename(compiled_binary));
  if (native && outputFile.slice(-4) !== ".exe") outputFile += ".exe"; // even on linux we add .exe for test clarity

  // loading json file for single test, e.g. test-http-create.js.json
  var obj = exports.checkJSON(src, tmpDir, native);
  if (obj) {

    var create = true;
    if (native && !obj.native && typeof obj.native !== "undefined")
      create = false;

    if (!native && !obj.package && typeof obj.package !== "undefined")
      create = false;

    if (!create) {
      if (fs.existsSync(outputFile)) fs.unlinkSync(outputFile);

      jx.rmdirSync(tmpDir);
      return;
    }
  }
  // this only moves previously rendered native files into file with exe
  // extension
  if (native && !fs.existsSync(outputFile)
    && fs.existsSync(outputFile.replace(".exe", "")))
    fs.renameSync(outputFile.replace(".exe", ""), outputFile);

  var jxp_file = path.join(dirname, basename + ".jxp");
  var dst = path.join(tmpDir, fname);
  // copying to temp dir
  jx.copyFileSync(src, dst);
  jx.copyFileSync(jxp_file, path.join(tmpDir, basename + ".jxp"));
  jx.copyFileSync(path.join(dirname, basename + ".out"), path.join(tmpDir,
    basename + ".out"));

  if (json[fname]
    && JSON.stringify(json[fname][suffix]) === JSON.stringify(versions)) {
    if (fs.existsSync(outputFile) && !exports.force_refresh) {
      return;
    }
  }

  if (!exports.silent)
    jxcore.utils.console.write("Creating", suffix, "for file:\t", src.replace(
      cwd, ""), "...", "yellow");

  process.chdir(tmpDir);

  var cmd = '"' + process.execPath + '" package ' + fname + ' ' + basename;
  if (fs.existsSync(jxp_file)) {
    cmd = '"' + process.execPath + '" compile ' + basename + '.jxp';
  } else {
    if (obj.slim)
      cmd += " -slim " + obj.slim;
  }

  if (fs.existsSync(compiled_binary)) fs.unlinkSync(compiled_binary);

  if (native) cmd += " -native";
  var res = jxcore.utils.cmdSync(cmd);

  process.chdir(cwd);

  if (!fs.existsSync(compiled_binary)) {
    jxcore.utils.console.log("Package was not created: " + compiled_binary
    + "\nFailed command: " + cmd, "\n", res.out, "red");
    return;
  } else {
    fs.renameSync(compiled_binary, outputFile);
    jx.copyFileSync(src + ".json", outputFile + ".json");
    jx.copyFileSync(src + ".jxcore.config", outputFile + ".jxcore.config");
    jx.copyFileSync(path.join(tmpDir, basename + ".jxp"), path.join(outputDir, basename + ".jxp"));
    jx.copyFileSync(path.join(tmpDir, basename + ".out"), path.join(outputDir, basename + ".out"));

    for (var o in obj.files_fs) {
      if (!obj.files_fs.hasOwnProperty(o))
        continue;
      var _src = path.join(tmpDir, obj.files_fs[o]);
      var _dst = path.join(outputDir, obj.files_fs[o]);
      jx.copySync(_src, _dst);
    }

    if (!exports.silent) jxcore.utils.console.log(" OK");

    if (exports.jx_json_enabled) {
      if (!json[fname]) {
        json[fname] = {};
      }
      json[fname][suffix] = versions;
      fs.writeFileSync(json_file, JSON.stringify(json, null, 4));
    }

    return outputFile;
  }
};

/**
 * loads json file for single test, e.g. test-http-create.js.json
 *
 * @param src_json
 */
exports.checkJSON = function (src, outputDir, native) {

  var src_json = src + ".json";

  var obj = {};

  try {
    if (fs.existsSync(src_json))
      obj = JSON.parse(fs.readFileSync(src_json).toString());
  } catch (ex) {
    jxcore.utils.console.log("Cannot read: ", src_json, "red");
    return;
  }

  var dirname = path.dirname(src); // eg. /.../test/jxcore/

  if (obj.dependencies) {

    if (!exports.silent)
      jxcore.utils.console.log("Installing dependencies: ", obj.dependencies,
        "yellow");

    var nd = path.join(dirname, "node_modules");
    if (!fs.existsSync(nd)) fs.mkdirSync(nd);

    var old = process.cwd();
    process.chdir(dirname);
    for (var dep in obj.dependencies) {
      if (!obj.dependencies.hasOwnProperty(dep))
        continue;
      var ret = jxcore.utils.cmdSync('"' + process.execPath + '" install '
      + obj.dependencies[dep]);
      if (ret.exitCode) console.error(ret.out);
    }
    process.chdir(old);
  }

  if (!obj.files) obj.files = [];

  obj.files.push("_assert.js");
  obj.files_fs = [];

  for (var o in obj.files) {
    if (!obj.files.hasOwnProperty(o))
      continue;

    var f = obj.files[o];
    var leave = f.slice(0, 1) === "&";
    if (leave) {
      f = f.slice(1);
      leave = f;
    }

    var _src = path.join(dirname, f);
    var _dst = path.join(outputDir, f);

    if (!fs.existsSync(_src)) continue;
    var stat = fs.statSync(_src);

    var d1 = path.dirname(_dst);
    var d2 = path.dirname(d1);
    var d3 = path.dirname(d2);

    var mkdir = function(d) {
      if (!fs.existsSync(d)) {
        fs.mkdirSync(d);
        if (leave)
          leave = path.basename(d);
      }
    };

    mkdir(d3);
    mkdir(d2);
    mkdir(d1);

    if (leave)
      obj.files_fs.push(leave);

    if (stat.isDirectory()) if (!fs.existsSync(_dst)) fs.mkdirSync(_dst);

    jx.copySync(_src, _dst);
  }

  if (native && obj.native !== false) {
    // excluding test file from native packaging, as it spawns process.execPath
    var str = fs.readFileSync(src).toString();
    if (str.indexOf("process.execPath") !== -1) {
      if (str.indexOf(".exec(") !== -1 || str.indexOf(".spawn") !== -1) {
        obj.native = false;
        jxcore.utils.console.log("Excluded", path.basename(src),
          "from native packaging, as it spawns process.execPath.",
          "yellow");
      }
    }
  }

  return obj;
};

/**
 * For e.g. name = "jxcore" and native = true, returns "_[auto]_jxcore_native"
 *
 * @param name
 * @param native
 */
exports.getOutputDirBasename = function (name, native) {

  var suffix = native ? "_native" : "_package";
  if (native === null) suffix = "";
  var prefix = name.slice(0, 6) === "_auto_" ? "" : "_auto_";

  return prefix + name + suffix;
};

/**
 * Returns output dir name for a given js file name
 *
 * @param js_file
 * @param native -
 *          bool. true for native package (-native), false for jx package
 * @return {*}
 * @example js_file = /.../test/jxcore/test-http-create.js native = false result =
 *          /.../test/jxcore_package/
 */
var getOutputDir = function (js_file, native) {
  var suffix = native ? "_native" : "_package";

  var old_ret = path.join(path.dirname(js_file) + suffix);
  var dir = path.dirname(js_file);
  var basename = path.basename(dir); // e.g. "jxcore"
  var ret = path.join(path.dirname(dir), exports.getOutputDirBasename(basename,
    native));

  if (fs.existsSync(old_ret)) fs.renameSync(old_ret, ret);

  if (!fs.existsSync(ret)) fs.mkdirSync(ret);

  return ret;
};

/**
 * Modifies jxcore/testcfg.py for targetDir and saves there as testcfg.py
 *
 * @param targetDir
 * @param name
 */
exports.renderTestCfg = function (targetDir, name) {
  var testcfg_py_src = path.join(__dirname, "jxcore/testcfg.py");
  var testcfg_py_dst = path.join(targetDir, "testcfg.py");
  var dir_basename = path.basename(targetDir);

  var isMessageDir = dir_basename.indexOf("messageMozJS") !== -1
    || dir_basename.indexOf("jxcore-message") !== -1;
  if (isMessageDir)
    testcfg_py_src = path.join(__dirname, "message/testcfg.py");

  if (!fs.existsSync(testcfg_py_src)) {
    jxcore.utils.console.log("Cannot find testcfg file: " + testcfg_py_src);
    return;
  }

  if (!name) name = "Test" + dir_basename;

  name = name.replace(/[\-\s]/g, "_");
  var str = fs.readFileSync(testcfg_py_src).toString().replace(/JXcore/g, name)
    .replace("'jxcore.status'", "'" + name.toLowerCase() + ".status'");

  if (isMessageDir) str = str.replace(/Message/g, name);

  fs.writeFileSync(testcfg_py_dst, str);
};

/**
 * Iterates through all js files located in dir and packs each of them into
 * separate jx or native package (that depends on -n/-p argv)
 *
 * @param dir
 * @example dir = /.../test/jxcore/ dir = /.../test/simple/
 */
exports.processSingleFolder = function (dir, jx_package, native_package) {

  if (!jx_package && !native_package) return;

  var files = fs.readdirSync(dir);

  for (var a = 0, len = files.length; a < len; a++) {

    if (files[a].slice(0, 1) == ".") continue;

    var _path = path.join(dir, files[a]);
    var stats = fs.statSync(_path);

    if (stats.isFile() && files[a].slice(-3) == ".js"
      && files[a].slice(0, 1) != "_") {
      if (jx_package) exports.createSinglePackage(_path, false);

      if (native_package) exports.createSinglePackage(_path, true);
    }
  }

  if (jx_package) {
    var outDir = getOutputDir(path.join(dir, "testcfg.py"), false);
    removeRedundantFiles(outDir, files);
    exports.renderTestCfg(outDir);
  }
  if (native_package) {
    var outDir = getOutputDir(path.join(dir, "testcfg.py"), true);
    removeRedundantFiles(outDir, files);
    exports.renderTestCfg(outDir);
  }
};

var removeRedundantFiles = function (dir, leaveFiles) {

  var files = fs.readdirSync(dir);

  for (var a = 0, len = files.length; a < len; a++) {
    var found = false;
    var f = files[a].replace(".jxp", "").replace(".exe", "")
      .replace(".json", "").replace(".jxcore.config", "").replace(".jx", "");

    if (f === files[a]) continue; // none of package files for removal

    for (var o in leaveFiles) {
      if (leaveFiles.hasOwnProperty(o) && leaveFiles[o] === f + ".js") {
        found = true;
        break;
      }
    }

    if (!found) {
      fs.unlinkSync(path.join(dir, files[a]));
    }
  }
};

exports.processFolders = function (dirArr, jx_package, native_package) {

  if (!dirArr.length) {
    dirArr = fs.readdirSync(__dirname);
  }

  for (var a = 0, len = dirArr.length; a < len; a++) {

    var dir = dirArr[a];
    var _path = __dirname + path.sep + dir;

    if (dir.slice(0, 1) == "." || dir.slice(-7) == "_native"
      || dir.slice(-8) == "_package") continue;

    var stats = fs.statSync(_path);

    if (stats.isDirectory()) {
      if (fs.existsSync(path.join(_path, "testcfg.py")))
        exports.processSingleFolder(_path, jx_package, native_package);
    }
  }

  process.chdir(cwd);
};
