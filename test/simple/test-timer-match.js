// Copyright & License details are available under JXCORE_LICENSE file

// timer uses Timer.now while a JS application might be depending Date.now
// This test case checks whether setTimeout etc. event fires sooner than expected

var last_time = Date.now() - 1;
var counter = 0;

var test = function() {
  var time = Date.now();

  if (time == last_time) {
    throw new Exception("Fail!");
  }

  if (++counter % 500 == 0) {
    console.log("]]", counter);

    if (counter >= 2000) {
      process.exit(0);
    }
  }

  last_time = Date.now();
  setTimeout(test, 1);
};

setTimeout(test, 1);