// Copyright & License details are available under JXCORE_LICENSE file


function tmp() {}
process.on('SIGINT', tmp);
process.removeListener('SIGINT', tmp);
setInterval(function() {
  process.stdout.write('keep alive\n');
}, 1000);