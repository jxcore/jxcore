// Copyright & License details are available under JXCORE_LICENSE file



var assert = require('assert');
var os = require('os');

var node_arch = ['arm', 'ia32', 'x64'];

assert.notStrictEqual(node_arch.indexOf(process.arch), -1, "Unknown value of process.arch: `" + process.arch + '`');
assert.notStrictEqual(node_arch.indexOf(os.arch()), -1, "Unknown value of os.arch(): `" + os.arch() + '`');

var node_platforms = ['darwin', 'freebsd', 'linux', 'sunos', 'win32', "android"];

assert.notStrictEqual(node_platforms.indexOf(process.platform), -1, "Unknown value of process.platform: `" + process.platform + '`');
assert.notStrictEqual(node_platforms.indexOf(os.platform()), -1, "Unknown value of os.platform(): `" + os.platform() + '`');