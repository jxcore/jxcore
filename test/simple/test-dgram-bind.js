// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var dgram = require('dgram');

var socket = dgram.createSocket('udp4');

socket.on('listening', function () {
  socket.close();
});

socket.bind(); // should not throw