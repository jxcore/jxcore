// Copyright Joyent, Inc. and other Node contributors.

if (!process.versions.openssl) {
  console.error("Skipping because node compiled without OpenSSL.");
  process.exit(0);
}
if (parseInt(process.versions.openssl[0]) < 1) {
  console.error("Skipping because node compiled with old OpenSSL version.");
  process.exit(0);
}

/* This will test a client using JUST a callback - in place of 
 * both a Identity & Key
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
var pskKey = new Buffer("d731ef57be09e5204f0b205b60627028", 'hex');

var clientCallback = function () {
  return {
    identity: identity,
    key: pskKey
  }
}


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
  serverResults.push(c.pskIdentity + ' ' + (c.authorized ? 'authorized' : 'not authorized'));
  c.once('data', function (data) {
    assert.equal(data.toString(), 'Hi.');
    c.write('Hi ' + c.pskIdentity);
    c.once('data', function (data) {
      assert.equal(data.toString(), 'Bye.');
    });
  });
});

server.listen(serverPort, startTest);

var options = {};
options.pskClientCallback = clientCallback;
options.ciphers = 'PSK-AES256-CBC-SHA:PSK-3DES-EDE-CBC-SHA';

function startTest() {
  console.log('connecting with callback.. %s', identity);
  var client = tls.connect(serverPort, 'localhost', options, function () {
    clientResults.push(client.pskIdentity + ' ' + (client.authorized ? 'authorized' : 'not authorized'));
    client.write('Hi.');
    client.on('data', function (data) {
      assert.equal(data.toString(), 'Hi ' + identity);
      client.end('Bye.');
      server.close();

    });
  });
  client.on('error', function (err) {
    console.log('connection as %s rejected', identity);
    clientResults.push(err.message);
  });
}


process.on('exit', function () {
  assert.deepEqual(serverResults, [
    'TestUser authorized']);
  assert.deepEqual(clientResults, [
    'TestUser authorized']);
});
