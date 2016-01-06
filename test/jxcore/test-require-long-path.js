// Copyright & License details are available under JXCORE_LICENSE file

var path = require('path');
var fs = require('fs');
var jxtools = require('jxtools');

var dir = path.join(__dirname, path.basename(__filename) + "-tmp-dir");

jxtools.rmdirSync(dir);
fs.mkdirSync(dir);

process.on('exit', function() {
  jxtools.rmdirSync(dir);
});

var len = 0;

var next = function() {
  len++;
  var fileName = new Array(len + 1).join('x') + '.js';
  var fullPath = path.join(dir, fileName);

  if (fullPath.length > 250) return;

  jxcore.utils.console.write('Try to require() file with path length = ' + fullPath.length);
  fs.writeFileSync(fullPath, 'exports.value = ' + fullPath.length + ';');
  try {
    require(fullPath);
    jxcore.utils.console.info(' OK');
  } catch(ex) {
    console.error('\n', ex);
  }
  fs.unlinkSync(fullPath);

  setTimeout(next, 1);
};

next();
