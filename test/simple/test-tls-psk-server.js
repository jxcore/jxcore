// Copyright & License details are available under JXCORE_LICENSE file


if (!process.versions.openssl) {
  console.error("Skipping because node compiled without OpenSSL.");
  process.exit(0);
}
if (parseInt(process.versions.openssl[0]) < 1) {
  console.error("Skipping because node compiled with old OpenSSL version.");
  process.exit(0);
}


var common = require('../common');
var assert = require('assert');

var join = require('path').join;
var net = require('net');
var fs = require('fs');
var crypto = require('crypto');
var tls = require('tls');
var spawn = require('child_process').spawn;
var exec = require('child_process').exec;

// versions of openssl do not support PSK
// Therefore we skip this
// test for all openssl versions less than 1.0.0.
function checkOpenSSL() {
  exec('openssl version', function(err, data) {;
    if (err) throw err;
    if (/OpenSSL 0\./.test(data)) {
      console.error('Skipping due to old OpenSSL version.');
      return false;
    }
    else {
      return true;
    }
  });
}

if (! checkOpenSSL()){
  console.error("Skipping because OpenSSL version < 1.0.0.");
  process.exit(0)
}


var connections = 0;
var pskey = "d731ef57be09e5204f0b205b60627028";
var identity = 'TestUser';

var PSKCiphers = 'PSK-AES256-CBC-SHA:PSK-3DES-EDE-CBC-SHA:PSK-AES128-CBC-SHA:PSK-RC4-SHA';

function log(a) {
  console.error('+++ server +++ ' + a);
}

var server = net.createServer(function(socket) {
  connections++;
  var isWin = /^win/.test(process.platform);
  if (!isWin)  log('connection fd=' + socket.fd);
  log('connectionKey: ' + socket.server._connectionKey);
  var sslcontext = crypto.createCredentials({});
  sslcontext.context.setCiphers(PSKCiphers);

  function serverCallback(id) {
    if (id == identity) {
      return new Buffer(pskey, 'hex');
    }
    return null;
  }
  sslcontext.context.setPskServerCallback(serverCallback);

  var pair = tls.createSecurePair(sslcontext, true);

  assert.ok(pair.encrypted.writable);
  assert.ok(pair.cleartext.writable);

  pair.encrypted.pipe(socket);
  socket.pipe(pair.encrypted);

  log('i set it secure');

  pair.on('secure', function() {
    log('connected+secure!');
    pair.cleartext.write('hello\r\n');
    log(pair.cleartext.getPeerCertificate());
    var cipher = pair.cleartext.getCipher();
    log('cipher name and version: ' + cipher.name + ':' + cipher.version);
  });

  pair.cleartext.on('data', function(data) {
    log('read bytes ' + data.length);
    pair.cleartext.write(data);
  });

  socket.on('end', function() {
    log('socket end');
  });

  pair.cleartext.on('error', function(err) {
    log('got error: ');
    log(err);
    log(err.stack);
    socket.destroy();
  });

  pair.encrypted.on('error', function(err) {
    log('encrypted error: ');
    log(err);
    log(err.stack);
    socket.destroy();
  });

  socket.on('error', function(err) {
    log('socket error: ');
    log(err);
    log(err.stack);
    socket.destroy();
  });

  socket.on('close', function(err) {
    log('socket closed');
  });

  pair.on('error', function(err) {
    log('secure error: ');
    log(err);
    log(err.stack);
    socket.destroy();
  });
});

var gotHello = false;
var sentWorld = false;
var gotWorld = false;
var opensslExitCode = -1;

server.listen(common.PORT, function() {
  var client = spawn('openssl', ['s_client',
                                 '-connect', '127.0.0.1:' + common.PORT,
                                 '-psk', pskey,
                                 '-cipher', PSKCiphers,
                                 '-psk_identity', identity]);

  var out = '';

  client.stdout.setEncoding('utf8');
  client.stdout.on('data', function(d) {
    out += d;

    if (!gotHello && /hello/.test(out)) {
      gotHello = true;
      client.stdin.write('world\r\n');
      sentWorld = true;
    }

    if (!gotWorld && /world/.test(out)) {
      gotWorld = true;
      client.stdin.end();
    }
  });

  client.stdout.pipe(process.stdout, { end: false });
  client.stderr.pipe(process.stderr, { end: false });

  client.on('exit', function(code) {
    opensslExitCode = code;
    server.close();
  });
});

process.on('exit', function() {
  assert.equal(1, connections);
  assert.ok(gotHello);
  assert.ok(sentWorld);
  assert.ok(gotWorld);
  assert.equal(0, opensslExitCode);
});
