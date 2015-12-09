// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var http = require('http');
var util = require('util');

var body = 'hello world\n';
var headers = {'connection': 'keep-alive'};

var server = http.createServer(function(req, res) {
  res.writeHead(200, {'Content-Length': body.length, 'Connection': 'close'});
  res.write(body);
  res.end();
});

var connectCount = 0;


server.listen(common.PORT, function() {
  var agent = new http.Agent({ maxSockets: 1 });
  var request = http.request({
    method: 'GET',
    path: '/',
    headers: headers,
    port: common.PORT,
    agent: agent
  }, function(res) {
    assert.equal(1, agent.sockets['localhost:' + common.PORT].length);
    res.resume();
  });
  request.on('socket', function(s) {
    s.on('connect', function() {
      connectCount++;
    });
  });
  request.end();

  request = http.request({
    method: 'GET',
    path: '/',
    headers: headers,
    port: common.PORT,
    agent: agent
  }, function(res) {
    assert.equal(1, agent.sockets['localhost:' + common.PORT].length);
    res.resume();
  });
  request.on('socket', function(s) {
    s.on('connect', function() {
      connectCount++;
    });
  });
  request.end();
  request = http.request({
    method: 'GET',
    path: '/',
    headers: headers,
    port: common.PORT,
    agent: agent
  }, function(response) {
    response.on('end', function() {
      assert.equal(1, agent.sockets['localhost:' + common.PORT].length);
      server.close();
    });
    response.resume();
  });
  request.on('socket', function(s) {
    s.on('connect', function() {
      connectCount++;
    });
  });
  request.end();
});

process.on('exit', function() {
  assert.equal(3, connectCount);
});