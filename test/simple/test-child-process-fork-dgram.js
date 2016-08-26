// Copyright & License details are available under JXCORE_LICENSE file

if (process.platform === 'win32') {
  console.error('Skipping: dgram sockets to child processes not supported on Windows.');
  process.exit(0);
}

var dgram = require('dgram');
var fork = require('child_process').fork;
var assert = require('assert');
var common = require('../common');

if (process.argv[2] === 'child') {
  var childCollected = 0;
  var server;

  process.on('message', function removeMe(msg, clusterServer) {
    if (msg === 'server') {
      server = clusterServer;

      server.on('message', function () {
        childCollected += 1;
      });

    } else if (msg === 'stop') {
      server.close();
      process.send(childCollected);
      process.removeListener('message', removeMe);
    }
  });

} else {
  var server = dgram.createSocket('udp4');
  var client = dgram.createSocket('udp4');
  var child = fork(__filename, ['child']);
  var msg = new Buffer('Some bytes');

  var parentCollected = 0;
  var childCollected = 0;
  server.on('message', function (msg, rinfo) {
    parentCollected += 1;
  });

  server.on('listening', function () {
    child.send('server', server);

    sendMessages();
  });

  var sendMessages = function () {
    var wait = 0;
    var send = 0;
    var total = 100;

    var timer = setInterval(function () {
      send += 1;
      if (send === total) {
        clearInterval(timer);
      }

      client.send(msg, 0, msg.length, common.PORT, '127.0.0.1', function(err) {
          if (err) throw err;

          wait += 1;
          if (wait === total) {
            shutdown();
          }
        }
      );
    }, 5);
  };

  var shutdown = function () {
    child.send('stop');
    child.once('message', function (collected) {
      childCollected = collected;
    });

    server.close();
    client.close();
  };

  server.bind(common.PORT, '127.0.0.1');

  process.once('exit', function () {
    assert(childCollected > 0);
    assert(parentCollected > 0);
  });
}