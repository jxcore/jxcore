// Copyright & License details are available under JXCORE_LICENSE file

var method = {
  define: function() {
    if (process.argv[1].indexOf('mt-keep:') >= 0 ||
        process.argv[1] == 'mt-keep') {
      process.keepAlive();
    }

    try {
      require.resolve(process.argv[2]);
    } catch (e) {
      console.error('could not resolve ', process.argv[2]);
      process.exit(1);
    }

    require(process.argv[2], null, null, true);
  }
};

var parsed = jxcore.utils.argv.parse();
var mter = parsed.mt || parsed['mt-keep'];

if (!mter || !mter.isInt) {
  console.log('mt/mt-keep count must be a number! (max:16)');
  console.log('');
  console.log('usage: mt:4');
  process.exit(1);
}

jxcore.tasks.setThreadCount(mter.asInt);

jxcore.tasks.on('message', function(m) {
  process.stdout.write(m);
});

jxcore.tasks.runOnce(method);
