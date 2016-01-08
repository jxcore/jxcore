var minz = require('../minimize');
var fs = require('fs');

var code = fs.readFileSync('./jquery-2.1.4.js') + "";
code = minz.minimize("jq.js", code, false);

fs.writeFileSync('./jq_tmp.js', code);
var res = jxcore.utils.cmdSync('jx jq_tmp.js');

if (res.exitCode) {
  console.log("FAILED! see jq_tmp.js\n");
  console.error(res.out);
  process.exit(1);
}

fs.unlinkSync('./jq_tmp.js');
