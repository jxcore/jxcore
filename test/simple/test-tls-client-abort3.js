// Copyright & License details are available under JXCORE_LICENSE file


if (!process.versions.openssl) {
  console.error('Skipping because node compiled without OpenSSL.');
  process.exit(0);
}

var common = require('../common');
var common = require('../common');
var tls = require('tls');
var fs = require('fs');
var assert = require('assert');

var options = {
  key: fs.readFileSync(common.fixturesDir + '/test_key.pem'),
  cert: fs.readFileSync(common.fixturesDir + '/test_cert.pem')
};

var gotError = 0,
    gotRequest = 0,
    connected = 0;

var server = tls.createServer(options, function(c) {
  gotRequest++;
  c.on('data', function(data) {
    console.log(data.toString());
  });

  c.on('close', function() {
    server.close();
  });
}).listen(common.PORT, function() {
  var c = tls.connect(common.PORT, { rejectUnauthorized: false }, function() {
    connected++;
    c.pair.ssl.shutdown();
    c.write('123');
    c.destroy();
  });

  c.once('error', function() {
    gotError++;
  });
});

process.once('exit', function() {
  assert.equal(gotError, 1);
  assert.equal(gotRequest, 1);
  assert.equal(connected, 1);
});