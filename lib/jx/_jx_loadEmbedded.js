// Copyright & License details are available under JXCORE_LICENSE file

var loaded = {};
var mod = process.binding("module_wrap");

exports.require = function(name) {
  if (loaded[name]) return loaded[name];

  var cc = {};
  mod.loadInternal(cc, name);

  loaded[name] = cc;

  return loaded[name];
};