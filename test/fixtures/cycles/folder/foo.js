// Copyright & License details are available under JXCORE_LICENSE file



var root = require('./../root');

exports.hello = function() {
  return root.calledFromFoo();
};