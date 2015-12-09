// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var http = require('http');
var util = require('util');


var body = 'hello world\n';

var server = http.createServer(function(req, res) {
  common.error('req: ' + req.method);
  res.writeHead(200, {'Content-Length': body.length});
  res.end();
  server.close();
});

var gotEnd = false;

server.listen(common.PORT, function() {
  var request = http.request({
    port: common.PORT,
    method: 'HEAD',
    path: '/'
  }, function(response) {
    common.error('response start');
    response.on('end', function() {
      common.error('response end');
      gotEnd = true;
    });
    response.resume();
  });
  request.end();
});

process.on('exit', function() {
  assert.ok(gotEnd);
});