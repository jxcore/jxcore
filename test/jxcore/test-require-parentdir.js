// Copyright & License details are available under JXCORE_LICENSE file

console.log("process.cwd()", process.cwd());
console.log("__filename", __filename);
console.log("__dirname", __dirname);


// raises exception when running as native package
var common = require("../common.js");
