// Copyright & License details are available under JXCORE_LICENSE file

if (!process.versions.openssl) {
  console.error('Skipping: node compiled without OpenSSL.');
  process.exit(0);
}

var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');
var path = require('path');

var options = {
  key: fs.readFileSync(path.join(common.fixturesDir, 'test_key.pem')),
  cert: fs.readFileSync(path.join(common.fixturesDir, 'test_cert.pem'))
};

var server = tls.Server(options, function(s) {
  assert.equal(s.address().address, s.socket.address().address);
  assert.equal(s.address().port, s.socket.address().port);

  assert.equal(s.remoteAddress, s.socket.remoteAddress);
  assert.equal(s.remotePort, s.socket.remotePort);
  s.end();
});

server.listen(common.PORT, '127.0.0.1', function() {
  assert.equal(server.address().address, '127.0.0.1');
  assert.equal(server.address().port, common.PORT);

  var c = tls.connect({
    host: '127.0.0.1',
    port: common.PORT,
    rejectUnauthorized: false
  }, function() {
    assert.equal(c.address().address, c.socket.address().address);
    assert.equal(c.address().port, c.socket.address().port);

    assert.equal(c.remoteAddress, '127.0.0.1');
    assert.equal(c.remotePort, common.PORT);
  });
  c.on('end', function() {
    server.close();
  });
});