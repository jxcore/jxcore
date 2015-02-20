// Copyright & License details are available under JXCORE_LICENSE file

var method = {
  define: function() {
    if (process.argv[1].indexOf('mt-keep:') >= 0
            || process.argv[1] == 'mt-keep') {
      process.keepAlive();
    }

    try {
      require.resolve(process.argv[2]);
    } catch(e) {
      console.error("could not resolve ", process.argv[2]);
      process.exit(-1);
    }
    
    require(process.argv[2], null, null, true);
  }
};

if (process.argv[1].indexOf(":") > 0) {
  var l = process.argv[1].split(':')[1];
  if (l && !l.length) l = null;
  try {
    l = parseInt(l != null ? l : "2");

    if (global.isNaN(l)) { throw new Error(""); }
  } catch (e) {
    console.log("mt/mt-keep count must be a number! (max:16)");
    console.log("");
    console.log("usage: mt:4");
    process.exit(0);
  }
  jxcore.tasks.setThreadCount(l);
} else
  jxcore.tasks.setThreadCount(2);

jxcore.tasks.on('message', function(m) {
  process.stdout.write(m);
});

jxcore.tasks.runOnce(method);