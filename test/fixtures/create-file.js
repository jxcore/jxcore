// Copyright & License details are available under JXCORE_LICENSE file


var fs = require('fs');

var file_name = process.argv[2];
var file_size = parseInt(process.argv[3]);

fs.truncateSync(file_name, file_size);