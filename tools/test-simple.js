var fs = require('fs');
var path = require('path');

var target = process.argv[2] || 'simple/'
var files = fs.readdirSync(target);

var mobile_skip_list =  [
  'test-child-',
  'test-cluster-',
  'test-repl-',
  'test-cli-',
  'test-stdin-',
];

function check_skip(fname) {
  if(jxcore.utils.OSInfo().isMobile) {
    for(var o in mobile_skip_list) {
      if (fname.indexOf(mobile_skip_list[o]) === 0) return true;
    }
  }

  return false;
}

var failed_count = 0;
for(var o in files) {
  var ext = path.extname(files[o]);

  if(check_skip(files[o])) continue;

  if(ext == '.js' && files[o].indexOf('test-') === 0) {
    jxcore.utils.console.write("Testing "+files[o], "green");
    var out = jxcore.utils.cmdSync('cd '+target+'; jx ' + files[o]);
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