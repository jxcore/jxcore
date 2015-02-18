// Copyright & License details are available under JXCORE_LICENSE file





// This must be run as root.

var common = require('../common');
var http = require('http'),
    https = require('https'),
    PORT = 80,
    SSLPORT = 443,
    assert = require('assert'),
    hostExpect = 'localhost',
    fs = require('fs'),
    path = require('path'),
    fixtures = path.resolve(__dirname, '../fixtures/keys'),
    options = {
      key: fs.readFileSync(fixtures + '/agent1-key.pem'),
      cert: fs.readFileSync(fixtures + '/agent1-cert.pem')
    };

http.createServer(function(req, res) {
  console.error(req.headers);
  assert.equal(req.headers.host, hostExpect);
  res.writeHead(200);
  res.end('ok');
  this.close();
}).listen(PORT);

https.createServer(options, function(req, res) {
  console.error(req.headers);
  assert.equal(req.headers.host, hostExpect);
  res.writeHead(200);
  res.end('ok');
  this.close();
}).listen(SSLPORT);

http
  .get({ host: 'localhost',
      port: PORT,
      headers: { 'x-port': PORT } })
  .on('response', function(res) {});

https
  .get({ host: 'localhost',
      port: SSLPORT,
      headers: { 'x-port': SSLPORT } })
  .on('response', function(res) {});