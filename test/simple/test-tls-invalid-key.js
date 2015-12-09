// Copyright & License details are available under JXCORE_LICENSE file


if (!process.versions.openssl) {
  console.error('Skipping because node compiled without OpenSSL.');
  process.exit(0);
}

var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');
var path = require('path');

var options = {
  key: fs.readFileSync(path.join(common.fixturesDir, 'keys/agent1-key.pem')),
  cert: fs.readFileSync(path.join(common.fixturesDir, 'test_cert.pem'))
};
var serverErrorHappened = false;
var clientErrorHappened = false;

var server = tls.Server(options, function(socket) {
  assert(false);
});
server.on('clientError', function(err) {
  serverErrorHappened = true;
  common.debug('Server: ' + err);
  server.close();
});

server.listen(common.PORT, function() {
  var client = tls.connect(common.PORT, function() {
    assert(false);
  });
  client.on('error', function(err) {
    clientErrorHappened = true;
    common.debug('Client: ' + err);
  });
});

process.on('exit', function() {
  assert(serverErrorHappened);
  assert(clientErrorHappened);
});