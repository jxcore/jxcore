// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating a package with -slim parameter (both jx and native).
 Then we check if slimmed folder is present in package, or not.
 */

if (process.IsEmbedded || exports.$JXP)
  return;


var jx = require('jxtools');
var assert = jx.assert;

var fs = require("fs");
var path = require("path");
var cp = require("child_process");

var removeOnExit = [];
var errors = [];
var started = 0;
var finished = 0;

console.time("total");

process.on("exit", function (code) {
  console.timeEnd("total");

  for (var o in removeOnExit)
    jx.rmdirSync(removeOnExit[o]);

  if (errors.length)
    throw new Error("\n" + errors.join("\n") + "\n");

  assert.strictEqual(started, finished, "Some tests did not finish!");
});


// creates a package in separate folder (dir)
var test_slim = function (definition, native) {

  started++;
  var dir = path.join(__dirname, path.basename(__filename) + "-tmp-dir-" + started);
  removeOnExit.push(dir);
  var native_name = process.platform === "win32" ? "my_package.exe" : "my_package";
  var jx_file = path.join(dir, native ? native_name : "my_package.jx");

  var slim = definition.slim;
  // makes an absolute path
  slim = slim.replace("#DIR#", dir);

  jx.rmdirSync(dir);
  fs.mkdirSync(dir);
  // we'll create a package from assets folder
  jx.copySync(path.join(__dirname, "assets"), dir);
  fs.writeFileSync(path.join(dir, "config.json"), JSON.stringify(definition, null, 4));

  // creating a package
  var cmd = 'cd "' + dir + '" && "' + process.execPath + '" package test-packaging-slim.js my_package -slim ' + slim + (native ? " -native" : "");
  cp.exec(cmd, {timeout: 30000}, function (error, stdout, stderr) {

    finished++;
    if (!fs.existsSync(jx_file)) {
      errors.push("Cannot find compiled package " + jx_file);
      return;
    }

    // removing all files except the created package
    var files = fs.readdirSync(dir);
    for (var o in files) {
      var f = path.join(dir, files[o]);
      if (f === jx_file) continue;
      var stat = fs.statSync(f);
      if (stat.isDirectory())
        jx.rmdirSync(f);
      else
        fs.unlinkSync(f);
    }

    var cmd = 'cd "' + dir + '" && "' + (native ? jx_file : '"' + process.execPath + '" ' + jx_file);
    var ret = jxcore.utils.cmdSync(cmd);
    if (ret.out.toString().length)
      errors.push(
        jxcore.utils.console.setColor("Errors for slim `" + slim + "` (", native ? "native" : "jx", "package ) :\n", "red") +
        jxcore.utils.console.setColor(jx_file, "yellow") +
        ":\n\t" + ret.out);
  });
};


// actual tests start here
// paths are taken from test/jxcore/assets folder

var definitions = [
  {
    slim: "src/a/b/src",
    should_be_readable: [
      "subfolder1/module1.js",
      "src/a/b/b1.js",
      "src/a/b/b1.txt"
    ],
    should_not_be_readable: [
      "src/a/b/src/src2.txt",
      "src/a/b/src/src2.js"
    ]
  },
  {
    slim: "./src/a/b/src",
    should_be_readable: [
      "subfolder1/module1.js",
      "src/a/b/b1.js",
      "src/a/b/b1.txt"
    ],
    should_not_be_readable: [
      "src/a/b/src/src2.txt",
      "src/a/b/src/src2.js"
    ]
  },
  {
    slim: "#DIR#/src/a/b/src",
    should_be_readable: [
      "subfolder1/module1.js",
      "src/a/b/b1.js",
      "src/a/b/b1.txt"
    ],
    should_not_be_readable: [
      "src/a/b/src/src2.txt",
      "src/a/b/src/src2.js"
    ]
  },
  {
    slim: "src,subfolder1",
    should_be_readable: [
      "assets1/file1.txt"
    ],
    should_not_be_readable: [
      "subfolder1/module1.js",
      "src/a/b/b1.js",
      "src/a/b/b1.txt",
      "src/a/b/src/src2.txt",
      "src/a/b/src/src2.js"
    ]
  },
  {
    slim: "subfolder1,#DIR#/src/a/b/src",
    should_be_readable: [
      "assets1/file1.txt"
    ],
    should_not_be_readable: [
      "subfolder1/module1.js",
      "src/a/b/b1.js",
      "src/a/b/b1.txt",
      "src/a/b/src/src2.txt",
      "src/a/b/src/src2.js"
    ]
  }
];


for (var o in definitions) {
  test_slim(definitions[o]);
  // sm native packaging is slow for now
  if (!process.versions.sm)
    test_slim(definitions[o], true);
}


