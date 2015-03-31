var reset = true;

jxcore.tasks.setThreadCount(4);

function test() {
  reset = false;
  var counter = 0;
  for (var i = 0; i < 20; i++) {
    jxcore.tasks.addTask(function (a) {
      require('./test-buffer-concat');
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
      if (counter == 20) {
        jxcore.tasks.unloadThreads();
        reset = true;
        console.log("RESET", Date.now());
      }
    });
  }
}

var ll = 0;
var inter = setInterval(function(){
  if (reset) {
    ll ++;
    if(ll>20) {
      clearInterval(inter);
      process.exit(0);
    } else {
      test();
    }
  }
},10);
