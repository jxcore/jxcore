var fs = require('fs');
var path = require('path');

var target = process.argv[2] || 'simple/'
var files = fs.readdirSync(target);

var failed_count = 0;
for(var o in files) {
  var ext = path.extname(files[o]);

  if(ext == '.js') {
    var out = jxcore.utils.cmdSync('cd '+target+'; jx ' + files[o]);
    jxcore.utils.console.write("Testing "+files[o], "green");
    if (out.exitCode != 0) {
      jxcore.utils.console.log(" [Failed]", "red");
      console.log(out.out);
      jxcore.utils.console.log("exit code", out.exitCode, "red");
      failed_count++;
      continue;
    }
    jxcore.utils.console.log(" [PASS]", "green");
  }
}

console.log("Total ", failed_count, "tests failed");