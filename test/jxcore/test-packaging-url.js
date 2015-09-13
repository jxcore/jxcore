// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit checks is `jx my_package.jx readme` and `jx my_package.jx license`
 gets correct url displayed either from jxp.website or package.json:repository.url.
 */

// -------------   init part

var jx = require('jxtools');
var assert = jx.assert;

var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var dir = __dirname + path.sep + path.basename(__filename) + "-tmp-dir" + path.sep;
var js_file = dir + "index.js";
var jxp_file = dir + "my_package.jxp";
var jx_file = dir + "my_package.jx";
var package_json_file = dir + "package.json";

var readme_file = dir + "README.MD";
var license_file = dir + "LICENSE";

process.on("exit", function (code) {
  jx.rmdirSync(dir);
  assert.strictEqual(tests.length, 0, "Some of the tests did not complete:" + tests.length);
});

// -------------   definition part

var jxp_template = {
  "name": "my_package",
  "version": "1.0",
  "author": "",
  "description": "",
  "company": "",
  "website": "http://jxcore.com/test_url_from_jxp_website.html",
  "package": null,
  "startup": "index.js",
  "execute": null,
  "extract": false,
  "output": "my_package.jx",
  "files": [
    "index.js"
  ],
  "assets": [
    "README.MD",
    "LICENSE"
  ],
  "license_file": "LICENSE",
  "readme_file": "README.MD"
};

var package_json_template = {
  "repository": {
    "type": "git",
    "url": "http://jxcore.com/test_url_from_package_json.html"
  }
};


var newJXP = function () {
  return JSON.parse(JSON.stringify(jxp_template));
};


var testPackage = function (jxp, cmd, expectedOutput) {

  jx.rmdirSync(dir);
  fs.mkdirSync(dir);

  var files = {};
  files[package_json_file] = JSON.stringify(package_json_template, null, 4);
  files[readme_file] = "This is some readme contents.";
  files[license_file] = "This is some license contents.";
  files[js_file] = "//nothing";
  files[jxp_file] = JSON.stringify(jxp, null, 4);

  // saving files
  for (var o in files)
    if (files.hasOwnProperty(o))
      fs.writeFileSync(o, files[o]);

  // compiling the jxp file
  cp.exec('"' + process.execPath + '" compile ' + path.basename(jxp_file), {
    timeout: 30000,
    cwd: dir
  }, function (error, stdout, stderr) {

    assert.ok(fs.existsSync(jx_file), "Cannot find compiled package " + jx_file);

    // now lets run the package and see, if contents was extracted
    var _cmd = '"' + process.execPath + '" ' + path.basename(jx_file) + ' ' + cmd;
    cp.exec(_cmd, {timeout: 30000, cwd: dir}, function (error, stdout, stderr) {

      var str = "" + stdout + stderr;
      if (str.indexOf(expectedOutput) === -1) {
        jxcore.utils.console.error("The following output:");
        jxcore.utils.console.error(stdout, "magenta");
        jxcore.utils.console.error("Does not contain:");
        jxcore.utils.console.error(expectedOutput, "yellow");
        jxcore.utils.console.error("Error:\n" + error);
      }

      next();
    });
  });
};


// async functions to be tested one by one
var tests = [
  function () {
    // no package.json: url should be displayed from jxp.website
    var jxp = newJXP();
    testPackage(jxp, "readme", jxp_template.website);
  },
  function () {
    // package.json exists (in "files"), but jxp.website has priority
    var jxp = newJXP();
    jxp.files.push("package.json");
    testPackage(jxp, "license", jxp_template.website);
  },
  function () {
    // package.json exists (in "files") and jxp.website is empty
    var jxp = newJXP();
    jxp.files.push("package.json");
    jxp.website = "";
    testPackage(jxp, "readme", package_json_template.repository.url);
  },
  function () {
    // package.json exists (in "assets"), but jxp.website has priority
    var jxp = newJXP();
    jxp.assets.push("package.json");
    testPackage(jxp, "license", jxp_template.website);
  },
  function () {
    // package.json exists (in "assets") and jxp.website is empty
    var jxp = newJXP();
    jxp.assets.push("package.json");
    jxp.website = "";
    testPackage(jxp, "readme", package_json_template.repository.url);
  }
];

// -------------   exec part

var next = function () {
  var fn = tests.shift();
  if (fn)
    fn()
};

next();

