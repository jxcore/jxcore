// Copyright & License details are available under JXCORE_LICENSE file

/*
 This is advanced test unit running tcp server and spawning child client.
 Client:
 - connects to server,
 - sends message to server
 - receives message from server
 - saves result in log file
 - and exits

 Server checks if everything was fine.
 */

var port = 8124;
var connected = false;
var serverReceived = false;
var clientReceived = false;

var jx = require('jxtools');
var assert = jx.assert;

var net = require('net');
var path = require('path');
var fs = require("fs");
var cp = require("child_process");

var log = process.cwd() + path.sep + "test-net-tcp-server-client.log";

if (fs.existsSync(log)) {
  fs.unlinkSync(log);
}

var strings = [
  "норм чё",
  " المتطرّف الأمريكية بحق. بل ضمنها المقاومة الاندونيسية",
  "諙 軿鉯頏 禒箈箑 聬蕡, 驧鬤鸕 袀豇貣 崣惝 煃, 螷蟞覮 鵳齖齘 肒芅邥 澂 嬼懫 鯦鯢鯡",
  "Εξπετενδα θχεωπηραστυς ατ μελ"
];

var client_to_srv = 1;
var srv_to_client = 2;


if (process.argv[process.argv.length - 1] != "client_test") {
  // this runs first, then it spawns child (client)
  var server = net.createServer(function (client) {

    client.setEncoding("utf8");
    client.on('data', function (data) {
      data = data.toString();

      serverReceived = data;
//            consol e.log("data from client: " + data);
      client.write(strings[srv_to_client] + '\r\n');
      client.pipe(client);
    });
    connected = true;
  });

  server.listen(port, function () {

    var cmd = '"' + process.execPath + '" mt-keep ' + __filename.replace(".js.jx", ".jx") + " client_test";
    var child = cp.exec(cmd, {timeout: 10000}, function (error, stdout, stderr) {
//            var out = stdout.toString() + stderr.toString();
//            consol e.log(out);

      // reading log written by client
      var exists = fs.existsSync(log);
      assert.ok(exists, "Cannot find log file created by client: " + log);

      if (exists) {
        var str = fs.readFileSync(log).toString();
        assert.strictEqual(str, "OK", "Client received : " + str + " but should receive: " + strings[srv_to_client]);
      }

      if (fs.existsSync(log)) {
        fs.unlinkSync(log);
      }
      setTimeout(process.exit, 3000).unref();
    });
  });


  process.on('exit', function (code) {
    assert.ok(connected, "Client did not connect to the server.");
    assert.strictEqual(serverReceived, strings[client_to_srv] + "\r\n", "Server received : " + serverReceived + " but should receive: " + strings[client_to_srv]);
  });


} else {

  // spawned client

  var client = net.connect({port: 8124}, function () {
    client.write(strings[client_to_srv] + "\r\n");
    client.end();
  });
  client.on('data', function (data) {
    data = data.toString();

    var status = (data === strings[srv_to_client] + "\r\n") ? "OK" : data;
    fs.writeFileSync(log, status);
    client.end();
    process.release();
  });
  client.on('error', function (err) {
    fs.writeFileSync(log, err);
  });

  setTimeout(function () {
    // apparently cannot connect
    process.exit(8);
  }, 3000);
}

