// Copyright & License details are available under JXCORE_LICENSE file

/*
 * This unit is testing availability of API members of JXcore objects.
 */

var jx = require('jxtools');
var assert = jx.assert;

var objects = {
  "process": {
    "keepAlive": "function",
    "release": "function",
    "sendToMain": "function",
    "subThread": "boolean",
    "threadId": "number",
    "unloadThread": "function"
  },
  "jxcore.tasks": {
    "addTask": "function",
    "forceGC": "function",
    "getThreadCount": "function",
    "jobCount": "function",
    "killThread": "function",
    "runOnce": "function",
    "setThreadCount": "function",
    "unloadThreads": "function"
  },
  "jxcore.monitor": {
    "followMe": "function",
    "leaveMe": "function"
  },
  "jxcore.store": {
    "exists": "function",
    "get": "function",
    "read": "function",
    "remove": "function",
    "set": "function"
  },
  "jxcore.store.shared": {
    "exists": "function",
    "expires": "function",
    "get": "function",
    "getBlockTimeout": "function",
    "read": "function",
    "remove": "function",
    "set": "function",
    "safeBlock": "function",
    "safeBlockSync": "function",
    "setBlockTimeout": "function",
    "setIfEqualsTo": "function",
    "setIfEqualsToOrNull": "function",
    "setIfNotExists": "function"
  }
};

for (var object in objects) {
  for (var member in objects[object]) {
    var type = objects[object][member];

    var name = object + "." + member;
    var current_type = typeof eval(name);

    assert.strictEqual(current_type, type, "The typeof '" + name + "' is '"
    + current_type + "' but should be '" + type + "'.");
  }
}

if (process.threadId !== -1) process.release();

jx.exitNowMT();