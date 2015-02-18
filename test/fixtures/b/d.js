// Copyright & License details are available under JXCORE_LICENSE file


console.error('load fixtures/b/d.js');

var string = 'D';

exports.D = function() {
  return string;
};

process.on('exit', function() {
  string = 'D done';
});