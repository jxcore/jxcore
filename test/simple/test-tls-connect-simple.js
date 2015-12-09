// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');

var clientConnected = 0;
var serverConnected = 0;

console.log("A");

var options = {
  key: fs.readFileSync(common.fixturesDir + '/keys/agent1-key.pem'),
  cert: fs.readFileSync(common.fixturesDir + '/keys/agent1-cert.pem')
};

console.log("A2");
var server = tls.Server(options, function(socket) {
console.log("B");
  if (++serverConnected === 2) {console.log("C");
    server.close();
  }
});

console.log("D");
server.listen(common.PORT, function() {console.log("E");
  var client1 = tls.connect({
    port: common.PORT,
    rejectUnauthorized: false
  }, function() {console.log("F");
    ++clientConnected;
    client1.end();
  });

console.log("G");
  var client2 = tls.connect({
    port: common.PORT,
    rejectUnauthorized: false
  });
  client2.on('secureConnect', function() {console.log("H");
    ++clientConnected;
    client2.end();
  });
});

console.log("J");
process.on('exit', function() {console.log("K");
  assert.equal(clientConnected, 2);
  assert.equal(serverConnected, 2);
});