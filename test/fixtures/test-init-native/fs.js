// Copyright & License details are available under JXCORE_LICENSE file


(function() {
  var fs = require('fs');
  if (fs.readFile) {
    require('util').print('fs loaded successfully');
  }
})();