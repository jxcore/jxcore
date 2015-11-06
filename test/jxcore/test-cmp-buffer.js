// Copyright & License details are available under JXCORE_LICENSE file

var jxt = process.binding('jxutils_wrap');
var fs = require('fs');
var jxools = require('jxtools');

if (!jxools.checkMinimumVersion('0.3.1.0'))
  return;


var strings = [
  "this is some contents",
  "норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ"
];


var test = function(sid, input) {
  var c = jxt._cmp(input);
  var output = jxt._ucmp(c);

  if (input.toString('base64') !== output.toString('base64')) {
    jxcore.utils.console.error('String are not equal for', sid, ":");
    jxcore.utils.console.info(input.slice(0, 200));
    jxcore.utils.console.warn(output.slice(0, 200));
    process.exit(1);
  }
};


test('test1', new Buffer(strings.join("\n")));
test('test2', fs.readFileSync(process.execPath));
