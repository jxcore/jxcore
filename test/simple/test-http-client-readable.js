// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var http = require('http');
var util = require('util');

var Duplex = require('stream').Duplex;

function FakeAgent() {
  http.Agent.call(this);

  this.createConnection = FakeAgent.prototype.createConnection;
}
util.inherits(FakeAgent, http.Agent);

FakeAgent.prototype.createConnection = function createConnection() {
  var s = new Duplex();

  function ondata(str)  {
    var buf = new Buffer(str);
    s.ondata(buf, 0, buf.length);
  }

  Object.defineProperty(s, 'ondata', {
    configurable: true,
    set: function(value) {
      Object.defineProperty(s, 'ondata', { value: value });

      process.nextTick(function() {
        ondata('HTTP/1.1 200 Ok\r\nTransfer-Encoding: chunked\r\n\r\n');

        s.readable = false;
        ondata('b\r\nhello world\r\n');
        ondata('b\r\n ohai world\r\n');
        ondata('0\r\n\r\n');
      });
    }
  });

  // Blackhole
  s._write = function write(data, enc, cb) {
    cb();
  };

  s.destroy = s.destroySoon = function destroy() {
    this.writable = false;
  };

  return s;
};

var received = '';
var ended = 0;
var response;

var req = http.request({
  agent: new FakeAgent()
}, function(res) {
  response = res;

  res.on('readable', function() {
    var chunk = res.read();
    if (chunk !== null)
      received += chunk;
  });

  res.on('end', function() {
    ended++;
  });
});
req.end();

process.on('exit', function() {
  assert.equal(received, 'hello world ohai world');
  assert.equal(ended, 1);
});
