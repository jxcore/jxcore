// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code is testing sending unicode strings with process.sendToThreads()
 */


var jx = require('jxtools');
var assert = jx.assert;

var strings = [
  "норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ"
];

var buf = new Buffer(250000);
buf.fill("0");
buf.write("Big string from buffer - 250 000 bytes.", 0);
//strings.push(buf.toString());


var received = [];
var cnt = 0;
var color = jxcore.utils.console.setColor;

var done = function() {
  for (var a = 0, len = strings.length; a < len; a++) {
    assert.ok(received[a], "Task did not receive this string:\n" + color("id = " + a + ": " + strings[a].slice(0, 255), "red"));
  }
  if (process.subThread)
    process.release();
};


// checking messages from other threads
jxcore.tasks.on('message', function (threadId, params) {
  cnt++;
  received[params.id] = params.str;
  assert.strictEqual(strings[params.id], params.str, "sentToThreads(): strings not equal: id = " + params.id + ": " + color(strings[params.id].slice(0, 255), "green") + " !== " + color(params.str.slice(0, 255), "red"));
  console.log("received", process.threadId, "from", process.threadId);

  if (cnt == strings.length * jxcore.tasks.getThreadCount()) {
    console.log("releasing", process.threadId);
    done();
  }
});

for (var a = 0, len = strings.length; a < len; a++) {
  process.sendToThreads({id: a, str: strings[a]});
}

// if done() was not called already...
setTimeout(done, 10000).unref();
