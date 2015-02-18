// Copyright & License details are available under JXCORE_LICENSE file

var binding = process.binding('os');
var util = require('util');

exports.endianness = binding.getEndianness;
exports.hostname = binding.getHostname;
exports.loadavg = binding.getLoadAvg;
exports.uptime = binding.getUptime;
exports.freemem = binding.getFreeMem;
exports.totalmem = binding.getTotalMem;
exports.cpus = binding.getCPUs;
exports.type = binding.getOSType;
exports.release = binding.getOSRelease;
exports.networkInterfaces = binding.getInterfaceAddresses;

exports.arch = function() {
  return process.arch;
};

exports.platform = function() {
  return process.platform;
};

exports.tmpdir = function() {
  return process.env.TMPDIR || process.env.TMP || process.env.TEMP
          || (process.platform === 'win32' ? 'c:\\windows\\temp' : '/tmp');
};

exports.tmpDir = exports.tmpdir;

exports.getNetworkInterfaces = util.deprecate(function() {
  return exports.networkInterfaces();
}, 'getNetworkInterfaces is now called `os.networkInterfaces`.');

exports.EOL = process.platform === 'win32' ? '\r\n' : '\n';