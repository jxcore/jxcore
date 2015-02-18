// Copyright & License details are available under JXCORE_LICENSE file


return;

var common = require('../common');
var script = common.fixturesDir + '/breakpoints_utf8.js';
process.env.NODE_DEBUGGER_TEST_SCRIPT = script;

require('./test-debugger-repl.js');