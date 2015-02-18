// Copyright & License details are available under JXCORE_LICENSE file


if (!process.versions.openssl) {
  console.error('Skipping because node compiled without OpenSSL.');
  process.exit(0);
}

var common = require('../common');
var assert = require('assert');
var https = require('https');
var fs = require('fs');
var path = require('path');

var options = {
  key: fs.readFileSync(path.join(common.fixturesDir, 'keys/agent1-key.pem')),
  cert: fs.readFileSync(path.join(common.fixturesDir, 'test_cert.pem'))
};
var serverErrorHappened = false;
var clientErrorHappened = false;

var server = https.Server(options, function(req, res) {
  assert(false);
});
server.on('clientError', function(err) {
  serverErrorHappened = true;
  common.debug('Server: ' + err);
  server.close();
});

server.listen(common.PORT, function() {
  var req = https.get({port: common.PORT}, function(res) {
    assert(false);
  });
  req.on('error', function(err) {
    clientErrorHappened = true;
    common.debug('Client: ' + err);
  });
});

process.on('exit', function() {
  assert(serverErrorHappened);
  assert(clientErrorHappened);
});