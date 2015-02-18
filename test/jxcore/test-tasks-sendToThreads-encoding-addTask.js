// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code is testing sending unicode strings with process.sendToThreads()
 */


var method = function (strings) {

  var assert = require('assert');

  process.keepAlive();
  var received = [];
  var cnt = 0;
  var color = jxcore.utils.console.setColor;

  // we don't want to wait too long, so we set a timeout
  var timer = setTimeout(function () {
    process.release();
    for (var a = 0, len = strings.length; a < len; a++) {
      assert.ok(received[a], "Task did not receive this string:\n" + color("id = " + a + ": " + strings[a].slice(0, 255), "red"));
    }

  }, 3000);

  // checking messages from other threads
  jxcore.tasks.on('message', function (threadId, params) {
    cnt++;
    received[params.id] = params.string;
    if (cnt == strings.length) {
      clearTimeout(timer);
      process.release();
    }

    assert.strictEqual(strings[params.id], params.string, "sentToThreads(): strings not equal: id = " + params.id + ": " + color(strings[params.id].slice(0, 255), "green") + " !== " + color(params.string.slice(0, 255), "red"));
  });

};

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


jxcore.tasks.addTask(method, strings, function (txt) {

  for (var a = 0, len = strings.length; a < len; a++) {
    process.sendToThreads({id: a, string: strings[a]});
  }

});

