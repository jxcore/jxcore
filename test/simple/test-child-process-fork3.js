// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var child_process = require('child_process');

child_process.fork(common.fixturesDir + '/empty.js'); // should not hang