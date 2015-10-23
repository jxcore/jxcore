// Copyright & License details are available under JXCORE_LICENSE file

// test only for Chakra builds
if (!process.versions.ch) return;

var assert = require('assert');

assert.ok(jxcore.uwp, 'jxcore.uwp should not be undefined');
var uwp = jxcore.uwp;

assert.ok(!uwp.projectNamespace("Windows"), 'uwp.projectNamespace should not return');

try {
  new Windows.Globalization.Calendar();
} catch(e) {
  console.error("This was not expected to throw!");  
  throw e;
}
uwp.close();