// Copyright Joyent, Inc. and other Node contributors.

if (!process.versions.openssl) {
  console.error("Skipping because node compiled without OpenSSL.");
  process.exit(0);
}
if (parseInt(process.versions.openssl[0]) < 1) {
  console.error("Skipping because node compiled with old OpenSSL version.");
  process.exit(0);
}

/* This will test a client using both Identity and NO Key - in place of 
 * a Client Callback
 *
*/


var common = require('../common');
var assert = require('assert');
var fs = require('fs');
var tls = require('tls');

var serverPort = common.PORT;

var serverResults = [];
var clientResults = [];


var identity = "TestUser";

//server ALWAYS requires cert regardles if not using
function filenamePEM(n) {
  return require('path').join(common.fixturesDir, 'keys', n + '.pem');
}

function loadPEM(n) {
  return fs.readFileSync(filenamePEM(n));
}

var serverOptions = {
  // configure a mixed set of cert and PSK ciphers
  ciphers: 'RC4-SHA:AES128-GCM-SHA256:AES128-SHA:AES256-SHA:PSK-AES256-CBC-SHA:PSK-3DES-EDE-CBC-SHA:PSK-AES128-CBC-SHA:PSK-RC4-SHA',
  pskCallback: function (id) {
    assert(id == identity);
    return pskKey ;
  },
  key: loadPEM('agent2-key'),
  cert: loadPEM('agent2-cert')
};

var server = tls.createServer(serverOptions, function (c) {
  console.log('%s connected', c.pskIdentity);
});

server.listen(serverPort, startTest);

var options = {};
options.pskIdentity = identity;
options.ciphers = 'PSK-AES256-CBC-SHA:PSK-3DES-EDE-CBC-SHA';

function startTest() {
  console.log('connecting with callback.. %s', identity);
  var client = tls.connect(serverPort, 'localhost', options, function () {
    client.on('data', function (data) {
      assert()      
      client.end('Bye.');
      server.close();
    });
  });
  client.on('error', function (err) {
    console.log('connection as %s rejected', identity);
    clientResults.push("connection rejected");
    console.log(err.message);
    serverResults.push(err.message);
    server.close();
  });
}

process.on('exit', function () {
  assert.deepEqual(clientResults, [
    'connection rejected']);
  assert(/error/g.test(serverResults[0]));
});
