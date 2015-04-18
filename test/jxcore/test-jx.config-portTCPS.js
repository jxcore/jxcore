// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is creating an https server and a client tries to connect to it.
 */

var jx = require('jxtools');
var assert = jx.assert;
var https = require("https");

var finished = false;
var port = 8126;
var portFromConfigFile = 8976; // <-- value from jxcore.config
var clientReceived = false;

var done = function() {
  assert.ok(finished, "Test unit did not finish.");
  var sid = "Thread id: " + process.threadId + ". ";
  assert.ok(clientReceived, sid + "Client did not receive message from the server on port " + portFromConfigFile);
  if (process.subThread)
    process.release();
};

var finish = function (req) {
  srv.unref();
  finished = true;
  if (req) {
    req.abort();
    done();
  }
};


// ########   server

var options = {
  key: "-----BEGIN RSA PRIVATE KEY-----" + "\n" +
  "MIIBOgIBAAJBAPVoqaY9CqK9fc0f185NiAaHKHD0fC0ue3wfkFPUt9p+KqhQrshM" + "\n" +
  "vzG25FnxJGoXFeBsmOpEpCDC/jmMfs2ygN0CAwEAAQJBAL1aN2QsPzOYcCPAiRwG" + "\n" +
  "WIlo6fxVuCaIcgEwvYThYca0BdZSBS11aatipuPat6zPhkskRMnwhsDUG/cp1n2/" + "\n" +
  "7SUCIQD9YVDQIZpjl/nbRwHeMLFTURgKTK2M06UtIN5pxxtkYwIhAPfyP1Tzizkt" + "\n" +
  "CwDEzPZjuYznNsoW/+iPxW9kYHlwcmm/AiBK3IzG4Za+5DETI1ie8B4EMsp6iIsi" + "\n" +
  "N4nu2m48LHKgZwIgNbUvqsMmgTgUjhITI7vuUWs0HwpkXIfoCH0BuMx2vBkCIBre" + "\n" +
  "vcgGGqBkTsmW5p/wJ5bjrONQo2udEtnoxjYsZE5h" + "\n" +
  "-----END RSA PRIVATE KEY-----",
  cert: "-----BEGIN CERTIFICATE-----" + "\n" +
  "MIIBfDCCASYCCQCEniBrwZX+iDANBgkqhkiG9w0BAQUFADBFMQswCQYDVQQGEwJQ" + "\n" +
  "TDETMBEGA1UECBMKU29tZS1TdGF0ZTEhMB8GA1UEChMYSW50ZXJuZXQgV2lkZ2l0" + "\n" +
  "cyBQdHkgTHRkMB4XDTE0MDQxNTE0NTAzMloXDTQxMDgzMDE0NTAzMlowRTELMAkG" + "\n" +
  "A1UEBhMCUEwxEzARBgNVBAgTClNvbWUtU3RhdGUxITAfBgNVBAoTGEludGVybmV0" + "\n" +
  "IFdpZGdpdHMgUHR5IEx0ZDBcMA0GCSqGSIb3DQEBAQUAA0sAMEgCQQD1aKmmPQqi" + "\n" +
  "vX3NH9fOTYgGhyhw9HwtLnt8H5BT1LfafiqoUK7ITL8xtuRZ8SRqFxXgbJjqRKQg" + "\n" +
  "wv45jH7NsoDdAgMBAAEwDQYJKoZIhvcNAQEFBQADQQCh1zAFb6QSOETPFDF0Plua" + "\n" +
  "zuXtPFXB2wTgZ76c2yr4oIhabZKY+JXrlnhYZ4u4qfQ189zjZDhZv0mPiACLTS6Z" + "\n" +
  "-----END CERTIFICATE-----"
};


var srv = https.createServer(options, function (req, res) {
  // sending back to client
  res.end("ok");
  finish();
});

srv.on('error', function (e) {
  assert.ok(0, "Server error: \n" + e);
  finish();
});

srv.on("listening", function () {
  client();
});
srv.listen(port, "localhost");


// ########   client

var client = function () {
  var options = {
    hostname: 'localhost',
    port: portFromConfigFile,
    path: '/',
    method: 'POST',
    rejectUnauthorized: false
  };

  var req = https.get(options, function () {
    clientReceived = true;
    finish(req);
  }).on("error", function (err) {
    assert.ok(0, "Client error: cannot connect on port defined in .jxcore.config (" + portFromConfigFile + ")\n" + err);
    finish(req);
  });
};

// if done() was not called already...
setTimeout(done, 10000).unref();

