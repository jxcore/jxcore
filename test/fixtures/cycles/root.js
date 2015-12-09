// Copyright & License details are available under JXCORE_LICENSE file



var foo = exports.foo = require('./folder/foo');

exports.hello = 'hello';
exports.sayHello = function() {
  return foo.hello();
};
exports.calledFromFoo = function() {
  return exports.hello;
};