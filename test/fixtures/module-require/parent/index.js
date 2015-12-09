// Copyright & License details are available under JXCORE_LICENSE file

var child = require('../child');
//console.log(child.module.require, child.module);
console.log(child.module.require('target'));
console.log(child.loaded);
exports.loaded = child.module.require('target');
