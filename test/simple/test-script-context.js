// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

var vm = require('vm');
var Script = vm.Script;
var script = new Script('"passed";');

common.debug('run in a new empty context');
var context = script.createContext();
var result = script.runInContext(context);
assert.equal('passed', result);

common.debug('create a new pre-populated context');
context = script.createContext({'foo': 'bar', 'thing': 'lala'});
assert.equal('bar', context.foo);
assert.equal('lala', context.thing);

common.debug('test updating context');
script = new Script('foo = 3;');
result = script.runInContext(context);
assert.equal(3, context.foo);
assert.equal('lala', context.thing);

// Issue GH-227:
Script.runInNewContext('', null, 'some.js');

// Issue GH-1140:
common.debug('test runInContext signature');
var gh1140Exception;
try {
  Script.runInContext('throw new Error()', context, 'expected-filename.js');
}
catch (e) {
  gh1140Exception = e;
  assert.ok(/expected-filename/.test(e.stack),
            'expected appearance of filename in Error stack');
}

assert.ok(gh1140Exception,
          'expected exception from runInContext signature test');

// GH-558, non-context argument segfaults / raises assertion
function isTypeError(o) {
  return o instanceof Error;
}

([undefined, null, 0, 0.0, '', {}, []].forEach(function(e) {
  assert.throws(function() { script.runInContext(e); }, isTypeError);
  assert.throws(function() { vm.runInContext('', e); }, isTypeError);
}));

// Issue GH-693:
common.debug('test RegExp as argument to assert.throws');
script = vm.createScript('var assert = require(\'assert\'); assert.throws(' +
                         'function() { throw "hello world"; }, /hello/);',
                         'some.js');
script.runInNewContext({ require : require });