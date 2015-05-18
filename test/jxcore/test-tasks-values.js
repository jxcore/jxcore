// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code tests if some of the evaluated values in a tasks are the same as in the mainthread
 */

var assert = require('assert');

// evaluates items given in an array
var method = function (sid) {

  //process.on("uncaughtException", function(err) {
  //    console.log("uncaughtException in " + process.argv[1] + "\n" + err);
  //});

  sid = sid || 'define()';

  var arr = [
    "__dirname",
    "__filename",
    "process.mainModule.filename",
    "process.argv",
    "process.cwd()",
    "process.execPath",
    "process.execArgv",
    "process.env",
    "process.arch",
    "process.platform"
  ];

  var arr2 = [
    __dirname,
    __filename,
    process.mainModule.filename,
    process.argv,
    process.cwd(),
    process.execPath,
    process.execArgv,
    process.env,
    process.arch,
    process.platform
  ];


  var util = require("util");
  var output = [];

  // evaluating
  for (var a = 0, len = arr.length; a < len; a++) {
    output.push(JSON.stringify(arr2[a]));
  }

  if (process.subThread) {
    process.sendToMain({sid: sid, ret: output, arr: arr});
  }

  if (process.threadId != -1)
    process.keepAlive(250);

  // returning value inside define() throws error
  if (sid !== "define()") {
    return {ret: output, arr: arr};
  }
};


jxcore.tasks.on('message', function (threadId, obj) {
  var ret = obj.ret;
  for (var a = 0, len = ret.length; a < len; a++) {
    assert.ok(typeof ret[a] !== "undefined", obj.sid + "Return array does not contain value for " + ret[a]);
    assert.strictEqual(ST.ret[a], ret[a], obj.sid + ": Value " + ST.arr[a] + " is:\n" + ret[a] + "\nwhile from MAIN thread is:\n" + ST.ret[a] + "\n\n");
  }
});

// first we run in single thread
var ST = method("main thread");

// now evaluating inside a tasks and comparing with ST evaluations
jxcore.tasks.addTask(method, "addTask as method");
jxcore.tasks.addTask({ define: function () { }, logic: method }, "logic()");
jxcore.tasks.addTask({define: method});