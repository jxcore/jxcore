'use strict';

var common = require('../common');
var assert = require('assert');

try {
  var crypto = require('crypto');
} catch (e) {
    console.error('Skipping: missing crypto.');
    process.exit(0);
}

var methods = ['SSLv2_method', 'SSLv2_client_method', 'SSLv2_server_method'];

methods.forEach(function(method) {
  assert.throws(function() {
    crypto.createCredentials({ secureProtocol: method });
  }, /SSLv2 methods disabled/);
});
