// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing if messages send with sendToMain() within a mainthread keep their encoding
 and if all of them has been received
 */

var assert = require('assert');

var strings = [
  "норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ"
];

var buf = new Buffer(250000);
buf.fill("0", 255, 250000);
buf.write("Big string from buffer - 250 000 bytes.", 0);
strings.push(buf.toString());

var received = {};

jxcore.tasks.on('message', function (tid, msg) {
  received[msg.id] = true;
  assert.strictEqual(strings[msg.id], msg.str, "sentToMain(): strings not equal: " + jxcore.utils.console.setColor(strings[msg.id].slice(0, 255), "green") + " !== " + jxcore.utils.console.setColor(msg.str.slice(0, 255), "red"))
});

process.on("exit", function (code) {
  for (var a = 0, len = strings.length; a < len; a++) {
    assert.ok(received[a], "Process did not receive this string:\n" + jxcore.utils.console.setColor("id = " + a + ": " + strings[a].slice(0, 255), "red"));
  }
});


for (var a = 0, len = strings.length; a < len; a++) {
  process.sendToMain({id: a, str: strings[a]});
}
