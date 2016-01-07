var fs = require('fs');
var path = require('path');
var parser = require('ecma-parser');
var tt = process.cwd() + "/compare.js";

var isFirst = true;
function test(dir) {
  var st = fs.statSync(dir);
  var files;
  var loc = dir;
  if (st.isDirectory())
    files = fs.readdirSync(dir);
  else {
    files = [dir];
    loc = "";
  }
  for (var i = 0, ln = files.length; i < ln; i++) {
    var file = path.join(loc, files[i]);
    var stat = fs.statSync(file);
    if (stat.isDirectory()) {
      isFirst = false;
      test(file);
    } else if (path.extname(file) == '.js') {
      try {
        var bl = parser.parse(file, fs.readFileSync(file) + "");

        fs.writeFileSync(tt, parser.blockToCode(bl));
        var res = jxcore.utils.cmdSync('diff ' + file + " " + tt);
        if (res.exitCode && res.out.trim().length) {
          jxcore.utils.console.log("FAIL", res.out, "\n\n", file, "red");
          process.exit(1);
        } else {
          jxcore.utils.console.log("PASS", file, "green");
        }
      } catch (e) {
        jxcore.utils.console.log("Error at file", file, "red");
        throw e;
      }
    }
    isFirst = false;
  }
}

test(process.argv[2]);
jxcore.utils.cmdSync('rm -rf ' + tt);