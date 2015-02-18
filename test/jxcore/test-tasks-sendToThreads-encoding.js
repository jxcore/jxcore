// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code is testing sending unicode strings with process.sendToThreads()
 */

// todo: check why this test fails when packaged


var method = function (strings) {

  var assert = require('assert');

  process.sendToMain("started");
  process.keepAlive();

  var received = [];
  var cnt = 0;
  var color = jxcore.utils.console.setColor;

  // we don't want to wait too long, so we set a timeout
  var timer = setTimeout(process.release, 3000);

  // checking messages from other threads
  jxcore.tasks.on('message', function (threadId, params) {
    cnt++;
    received[params.id] = params.str;
    if (cnt == strings.length) {
      clearTimeout(timer);
      process.release();
    }

    assert.strictEqual(strings[params.id], params.str, "sentToThreads(): strings not equal: id = " + params.id + ": " + color(strings[params.id].slice(0, 255), "green") + " !== " + color(params.str.slice(0, 255), "red"));
  });

  process.on('exit', function (code) {

    for (var a = 0, len = strings.length; a < len; a++) {
      assert.ok(received[a], "Task from thread " + process.threadId + " did not receive this string:\n" + color("id = " + a + ": " + strings[a].slice(0, 255), "red"));
    }
  });

};

var strings = [
  "норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ"
];

var buf = new Buffer(250000);
//buf.fill("0");
buf.write("Big string from buffer - 250 000 bytes.", 0);
strings.push(buf.toString());


var cnt = 5;
jxcore.tasks.setThreadCount(cnt);
jxcore.tasks.runOnce(method, strings);


jxcore.tasks.on('message', function (tid, msg) {

  cnt--;
  // making sure, that all tasks are already running, before we start sending messages
  if (!cnt) {
    for (var a = 0, len = strings.length; a < len; a++) {
      process.sendToThreads({id: a, str: strings[a]});
    }
  }
});
