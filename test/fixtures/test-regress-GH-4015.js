// Copyright & License details are available under JXCORE_LICENSE file

var fs = require('fs');

function load() {
  fs.statSync('.');
  load();
}
load();
