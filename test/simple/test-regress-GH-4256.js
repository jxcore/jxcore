// Copyright & License details are available under JXCORE_LICENSE file


process.domain = null;
timer = setTimeout(function() {
  console.log("this console.log statement should not make node crash");
}, 1);