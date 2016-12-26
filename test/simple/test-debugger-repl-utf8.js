// Copyright & License details are available under JXCORE_LICENSE file

console.error('Skipping: not supported on any platform.');
process.exit(0);

var common = require('../common');
var script = common.fixturesDir + '/breakpoints_utf8.js';
process.env.NODE_DEBUGGER_TEST_SCRIPT = script;

require('./test-debugger-repl.js');