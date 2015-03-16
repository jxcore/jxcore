// Copyright & License details are available under JXCORE_LICENSE file

var https = require('https');

https.get('https://s3.amazonaws.com/', function(res) {
  res.resume();
}).end();
