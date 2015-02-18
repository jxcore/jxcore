// Copyright & License details are available under JXCORE_LICENSE file

/*
 This code was generating Segmentation fault: 11
 It checks if store.shared is working and accessible from the subbthread
 */


var jx = require('jxtools');
var assert = jx.assert;
jxcore.store.shared.set(5, "echo");

jxcore.tasks.on('emptyQueue', function () {
  assert.strictEqual(jxcore.store.shared.exists(5), false, "The item exists, but was deleted");
});


var method = function (param) {

  var jx = require('jxtools');
  var assert = jx.assert;
  var v = jxcore.store.shared.read(5);
  assert.strictEqual(v, "echo", "Value is not the same. Should be 'echo', but is " + v);

  jxcore.store.shared.remove(5);
};

jxcore.tasks.addTask(method);