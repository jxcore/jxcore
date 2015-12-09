// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var dgram = require('dgram');

dgram.createSocket('udp4').bind(common.PORT + 0, common.mustCall(function() {
  assert.equal(this.address().port, common.PORT + 0);
  assert.equal(this.address().address, '0.0.0.0');
  this.close();
}));

dgram.createSocket('udp6').bind(common.PORT + 1, common.mustCall(function() {
  assert.equal(this.address().port, common.PORT + 1);
  var address = this.address().address;
  if (address === '::ffff:0.0.0.0')
    address = '::';
  assert.equal(address, '::');
  this.close();
}));