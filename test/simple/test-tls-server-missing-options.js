// Copyright & License details are available under JXCORE_LICENSE file

if (!process.versions.openssl) {
  console.error('Skipping: node compiled without OpenSSL.');
  process.exit(0);
}

var common = require('../common');
var assert = require('assert');
var https = require('https');
var tls = require('tls');

assert.throws(function() {
  tls.createServer({ /* empty */}).listen(0);
}, /missing.+certificate/i);

assert.throws(function() {
  https.createServer({ /* empty */}).listen(0);
}, /missing.+certificate/i);