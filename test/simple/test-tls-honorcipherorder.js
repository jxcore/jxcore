// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');
var nconns = 0;
// test only in TLSv1 to use DES which is no longer supported TLSv1.2
// to be safe when the default method is updated in the future
var SSL_Method = 'TLSv1_method';
var localhost = '127.0.0.1';

process.on('exit', function() {
  assert.equal(nconns, 4);
});

function test(honorCipherOrder, clientCipher, expectedCipher, cb) {
  var soptions = {
    secureProtocol: SSL_Method,
    key: fs.readFileSync(common.fixturesDir + '/keys/agent2-key.pem'),
    cert: fs.readFileSync(common.fixturesDir + '/keys/agent2-cert.pem'),
    ciphers: 'AES256-SHA:RC4-SHA:DES-CBC-SHA',
    honorCipherOrder: !!honorCipherOrder
  };

  var server = tls.createServer(soptions, function(cleartextStream) {
    nconns++;
  });
  server.listen(common.PORT, localhost, function() {
    var coptions = {
      rejectUnauthorized: false,
      secureProtocol: SSL_Method
    };
    if (clientCipher) {
      coptions.ciphers = clientCipher;
    }
    var client = tls.connect(common.PORT, localhost, coptions, function() {
      var cipher = client.getCipher();
      client.end();
      server.close();
      assert.equal(cipher.name, expectedCipher);
      if (cb) cb();
    });
  });
}

test1();

function test1() {
  // Client has the preference of cipher suites by default
  test(false, 'DES-CBC-SHA:RC4-SHA:AES256-SHA','DES-CBC-SHA', test2);
}

function test2() {
  // Server has the preference of cipher suites where AES256-SHA is in
  // the first.
  test(true, 'DES-CBC-SHA:RC4-SHA:AES256-SHA', 'AES256-SHA', test3);
}

function test3() {
  // Server has the preference of cipher suites. RC4-SHA is given
  // higher priority over DES-CBC-SHA among client cipher suites.
  test(true, 'DES-CBC-SHA:RC4-SHA', 'RC4-SHA', test4);
}

function test4() {
  // As client has only one cipher, server has no choice in regardless
  // of honorCipherOrder.
  test(true, 'DES-CBC-SHA', 'DES-CBC-SHA');
}