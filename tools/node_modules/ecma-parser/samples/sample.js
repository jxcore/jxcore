var parser = require('ecma-parser');

var js_code = "\
  var x = 1;\
  var y = 2;\
  {\
    console.log(x + y);\
  }\
";

var bl = parser.parse("test.js", js_code);
parser.printBlocks(bl.subs[bl.subs.length-2]);