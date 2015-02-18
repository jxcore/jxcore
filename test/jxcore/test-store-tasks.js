// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code test store and store.shared from inside a subthread.
 Also checks if value written in store does not belong to store.shared.
 */


var q = {};

q.define = function () {
  var store = jxcore.store;
  var shared = jxcore.store.shared;

  var jx = require('jxtools');
  var assert = jx.assert;

  store.set(1, "one");
  store.set("counter", 0);
};

q.logic = function (obj) {

  var s = ". ThreadID: " + process.threadId + ", task id: " + obj.id;

  var v = store.read("1");
  assert.strictEqual(v, "one", "Value in jxcore.store should be 'one', but is " + v + s);

  store.set("counter", 999);

  var isInShared = shared.exists("counter");
  assert.strictEqual(isInShared, false);
};

var obj = {dirname: __dirname + "/"};

for (var a = 1; a <= 10; a++) {
  obj.id = a;
  jxcore.tasks.addTask(q, obj);
}
