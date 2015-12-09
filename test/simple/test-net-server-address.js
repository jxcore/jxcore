// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var net = require('net');

// Test on IPv4 Server
var localhost_ipv4 = '127.0.0.1';
var family_ipv4 = 'IPv4';
var server_ipv4 = net.createServer();

server_ipv4.on('error', function(e) {
  console.log('Error on ipv4 socket: ' + e.toString());
});

server_ipv4.listen(common.PORT, localhost_ipv4, function() {
  var address_ipv4 = server_ipv4.address();
  assert.strictEqual(address_ipv4.address, localhost_ipv4);
  assert.strictEqual(address_ipv4.port, common.PORT);
  assert.strictEqual(address_ipv4.family, family_ipv4);
  server_ipv4.close();
});

// Test on IPv6 Server
var localhost_ipv6 = '::1';
var family_ipv6 = 'IPv6';
var server_ipv6 = net.createServer();

server_ipv6.on('error', function(e) {
  console.log('Error on ipv6 socket: ' + e.toString());
});

server_ipv6.listen(common.PORT, localhost_ipv6, function() {
  var address_ipv6 = server_ipv6.address();
  assert.strictEqual(address_ipv6.address, localhost_ipv6);
  assert.strictEqual(address_ipv6.port, common.PORT);
  assert.strictEqual(address_ipv6.family, family_ipv6);
  server_ipv6.close();
});