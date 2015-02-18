// Copyright & License details are available under JXCORE_LICENSE file

var loaded = {};
var mod = process.binding("module_wrap");

exports.load = function(name) {
  if (loaded[name]) return loaded[name];

  var cc = {
    exports: {}
  };
  mod.loadInternal(cc, name);

  loaded[name] = cc.exports;

  return loaded[name];
};