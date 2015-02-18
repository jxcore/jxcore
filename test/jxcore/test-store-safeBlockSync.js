// Copyright & License details are available under JXCORE_LICENSE file

// to be completed

var jx = require('jxtools');
var assert = jx.assert;

var key = "my_key";
var shared = jxcore.store.shared;

var run = function () {
  shared.safeBlockSync(key, function () {

    if (!shared.exists(key)) {
      shared.set(key, 0);
    }

    var n = shared.read(key);
    n = parseInt(n) + 1;
    shared.set(key, n);
  });
};

run();
// now value of the key should be 1
assert.strictEqual(shared.read(key), "1", 'Value for the key `' + key + '` is ' + shared.read(key) + " but should be 1");


run();
// now value of the key should be 2
assert.strictEqual(shared.read(key), "2", 'Value for the key `' + key + '` is ' + shared.read(key) + " but should be 2");