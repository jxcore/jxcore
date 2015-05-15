// Copyright & License details are available under JXCORE_LICENSE file


if (process.isPackaged)
  return;

var assert = require('assert'),
  jxtools = require("jxtools");

jxtools.listenForSignals();

var cmd = '"' + process.execPath + '" monitor ';

// kill monitor if it stays as dummy process
jxcore.utils.cmdSync(cmd + "stop");

process.on('exit', function (code) {
  jxcore.utils.cmdSync(cmd + 'stop');
  var _cmd = process.platform == 'win32' ? 'del /q ' : 'rm -f ';
  jxcore.utils.cmdSync(_cmd + "*monitor*.log");
});

var arr = [
  cmd + "start",
  cmd + "stop"
];

for (var o in arr) {
  var res = jxcore.utils.cmdSync(arr[o]);
  var splited = res.out.split("\n");

  assert.ok(splited.length >= 2, "Output from running the command " + arr[o] + " is probably shorter: \n" + JSON.stringify(res, null, 4));
}