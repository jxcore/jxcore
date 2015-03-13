// Copyright & License details are available under JXCORE_LICENSE file


/*
 * This test is a regression test for joyent/node#8897.
 */

var common = require('../common');
var assert = require('assert');
var net = require('net');

var clients = [];

var server = net.createServer(function onClient(client) {
  clients.push(client);

  if (clients.length === 2) {
    /*
     * Enroll two timers, and make the one supposed to fire first
     * unenroll the other one supposed to fire later. This mutates
     * the list of unref timers when traversing it, and exposes the
     * original issue in joyent/node#8897.
     */
    clients[0].setTimeout(1, function onTimeout() {
      clients[1].setTimeout(0);
      clients[0].end();
      clients[1].end();
    });

    // Use a delay that is higher than the lowest timer resolution accross all
    // supported platforms, so that the two timers don't fire at the same time.
    clients[1].setTimeout(50);
  }
});

server.listen(common.PORT, '127.0.0.1', function() {
  var nbClientsEnded = 0;

  function addEndedClient(client) {
    ++nbClientsEnded;
    if (nbClientsEnded === 2) {
      server.close();
    }
  }

  var client1 = net.connect({ port: common.PORT })
  client1.on('end', addEndedClient);

  var client2 = net.connect({ port: common.PORT });
  client2.on('end', addEndedClient);
});
