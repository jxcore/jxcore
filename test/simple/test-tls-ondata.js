// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');

var options = {
  key: fs.readFileSync(common.fixturesDir + '/keys/agent1-key.pem'),
  cert: fs.readFileSync(common.fixturesDir + '/keys/agent1-cert.pem')
};

var server = tls.Server(options, function(socket) {
  socket.end('hello world');
});

server.listen(common.PORT, function() {
  var client = tls.connect({
    port: common.PORT,
    rejectUnauthorized: false
  });
  // test that setting the `ondata` function *prevents* data from
  // being pushed to the streams2 interface of the socket
  client.ondata = function (buf, start, length) {
    var b = buf.slice(start, length);
    process.nextTick(function () {
      var b2 = client.read();
      if (b2) {
        assert.notEqual(b.toString(), b2.toString());
      }
      client.destroy();
      server.close();
    });
  };
});