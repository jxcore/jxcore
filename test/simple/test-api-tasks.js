// Copyright & License details are available under JXCORE_LICENSE file


var reset = true;

jxcore.tasks.setThreadCount(2);

  var counter = 0;
function test() {
  reset = false;
  for (var i = 0; i < 10; i++) {
    jxcore.tasks.addTask(function (a) {
      require('fs').ReadStream(__filename)
				.on('data', function (d) {})
				.on('end', function () {
					console.log("finished");
					process.release();
			});

      var str = "";
      for(var o in process.env) {
        str += o + " : " + process.env[o];
      }
      var buffer = new Buffer(str);
      buffer.fill(65);
      jxcore.tasks.forceGC();
      for(var i = 0; i< 1e4; i++)
        if (i % 13 == a+147) break;

      return a + buffer[a];
    }, i, function (retval) {
        counter++;
      if (counter == 10) {
        counter = 0;
        jxcore.tasks.unloadThreads();
        
        // we need a callback for threads unloaded!
        setTimeout(function(){
          reset = true;
        }, 500);
        console.log("RESET", Date.now());
      }
    });
  }
}

var ll = 0;
var inter = setInterval(function(){
  if (reset) {
    ll ++;
    if(ll>10) {
      clearInterval(inter);
      process.exit(0);
    } else {
      test();
    }
  }
},10);
