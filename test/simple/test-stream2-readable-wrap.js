// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');

var Readable = require('_stream_readable');
var Writable = require('_stream_writable');
var EE = require('events').EventEmitter;

var testRuns = 0, completedRuns = 0;
function runTest(highWaterMark, objectMode, produce) {
  testRuns++;

  var old = new EE;
  var r = new Readable({ highWaterMark: highWaterMark, objectMode: objectMode });
  assert.equal(r, r.wrap(old));

  var ended = false;
  r.on('end', function() {
    ended = true;
  });

  var pauses = 0;
  var resumes = 0;

  old.pause = function() {
    pauses++;
    old.emit('pause');
    flowing = false;
  };

  old.resume = function() {
    resumes++;
    old.emit('resume');
    flow();
  };

  var flowing;
  var chunks = 10;
  var oldEnded = false;
  var expected = [];
  function flow() {
    flowing = true;
    while (flowing && chunks-- > 0) {
      var item = produce();
      expected.push(item);
      console.log('emit', chunks);
      old.emit('data', item);
    }
    if (chunks <= 0) {
      oldEnded = true;
      console.log('old end', chunks, flowing);
      old.emit('end');
    }
  }

  var w = new Writable({ highWaterMark: highWaterMark * 2, objectMode: objectMode });
  var written = [];
  w._write = function(chunk, encoding, cb) {
    console.log(chunk);
    written.push(chunk);
    setTimeout(cb);
  };

  w.on('finish', function() {
    completedRuns++;
    performAsserts();
  });

  r.pipe(w);

  flow();

  function performAsserts() { 
    assert(ended);
    assert(oldEnded);
    assert.deepEqual(written, expected);
    assert.equal(pauses, 10);
    assert.equal(resumes, 9);
  }
}

runTest(10, false, function(){ return new Buffer('xxxxxxxxxx'); });
runTest(1, true, function(){ return { foo: 'bar' }; });

process.on('exit', function() {
  assert.equal(testRuns, completedRuns);
  console.log('ok');
});