// Copyright & License details are available under JXCORE_LICENSE file


var port = 8124;
var clientConnected = false;
var serverReceived = false;
var clientReceived = false;

var jx = require('jxtools');
var assert = jx.assert;
var net = require('net');
var path = require('path');
var fs = require("fs");
var cp = require("child_process");

var strings = [
  "норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ"
];

var client_to_srv = 1;
var srv_to_client = 2;

var received = 0;


//consol e.logOrg("threads", jxcore.tasks.getThreadCount(),"threadId", process.threadId);

var server = net.createServer(function (client) {

  // this callback doesn't has to be called in the same thread as client's call

  client.setEncoding("utf8");
  client.on('data', function (data) {
    serverReceived = data.toString();
    client.write(strings[srv_to_client] + '\r\n');
    client.pipe(client);

    var sid = "Thread id: " + process.threadId + ". ";
    assert.strictEqual(serverReceived, strings[client_to_srv] + "\r\n", sid + "Server received : " + serverReceived + " but should receive: " + strings[client_to_srv]);
  });

});

server.listen(port, function () {

  var client = net.connect({port: 8124}, function () {
    client.write(strings[client_to_srv] + "\r\n");
    client.end();
  });
  client.on('data', function (data) {
    clientReceived = data.toString();
    clientConnected = true;
    if (process.threadId !== -1)
      process.release();
    if (process.threadId === -1)
      server.close();
  });
});


process.on('exit', function (code) {
//    consol e.log("exit on " + process.threadId);
  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(clientConnected, sid + "Client did not connect to the server.");
  assert.strictEqual(clientReceived, strings[srv_to_client] + "\r\n", sid + "Client received : " + clientReceived + " but should receive: " + strings[srv_to_client]);
});
