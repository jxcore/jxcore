// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

var expectFilePath = process.platform === 'win32' ||
                     process.platform === 'linux' ||
                     process.platform === 'darwin';

var watchSeenOne = 0;
var watchSeenTwo = 0;
var watchSeenThree = 0;

var testDir = common.tmpDir;

var filenameOne = 'watch.txt';
var filepathOne = path.join(testDir, filenameOne);

var filenameTwo = 'hasOwnProperty';
var filepathTwo = filenameTwo;
var filepathTwoAbs = path.join(testDir, filenameTwo);

var filenameThree = 'newfile.txt';
var testsubdir = path.join(testDir, 'testsubdir');
var filepathThree = path.join(testsubdir, filenameThree);


process.on('exit', function() {
  assert.ok(watchSeenOne > 0);
  assert.ok(watchSeenTwo > 0);
  assert.ok(watchSeenThree > 0);
});

// Clean up stale files (if any) from previous run.
try { fs.unlinkSync(filepathOne); } catch (e) { }
try { fs.unlinkSync(filepathTwoAbs); } catch (e) { }
try { fs.unlinkSync(filepathThree); } catch (e) { }
try { fs.rmdirSync(testsubdir); } catch (e) { }

fs.writeFileSync(filepathOne, 'hello');

assert.doesNotThrow(
    function() {
      var watcher = fs.watch(filepathOne)
      watcher.on('change', function(event, filename) {
        assert.equal('change', event);

        // darwin only shows the file path for subdir watching,
        // not for individual file watching.
        if (expectFilePath && process.platform !== 'darwin') {
          assert.equal('watch.txt', filename);
        } else {
          assert.equal(null, filename);
        }
        watcher.close();
        ++watchSeenOne;
      });
    }
);

setTimeout(function() {
  fs.writeFileSync(filepathOne, 'world');
}, 1000);


process.chdir(testDir);

fs.writeFileSync(filepathTwoAbs, 'howdy');

assert.doesNotThrow(
    function() {
      var watcher = fs.watch(filepathTwo, function(event, filename) {
        assert.equal('change', event);

        // darwin only shows the file path for subdir watching,
        // not for individual file watching.
        if (expectFilePath && process.platform !== 'darwin') {
          assert.equal('hasOwnProperty', filename);
        } else {
          assert.equal(null, filename);
        }
        watcher.close();
        ++watchSeenTwo;
      });
    }
);

setTimeout(function() {
  fs.writeFileSync(filepathTwoAbs, 'pardner');
}, 1000);

try { fs.unlinkSync(filepathThree); } catch (e) {}
try { fs.mkdirSync(testsubdir, 0700); } catch (e) {}

assert.doesNotThrow(
    function() {
      var watcher = fs.watch(testsubdir, function(event, filename) {
        var renameEv = process.platform === 'sunos' ? 'change' : 'rename';
        assert.equal(renameEv, event);
        if (expectFilePath) {
          assert.equal('newfile.txt', filename);
        } else {
          assert.equal(null, filename);
        }
        watcher.close();
        ++watchSeenThree;
      });
    }
);

setTimeout(function() {
  var fd = fs.openSync(filepathThree, 'w');
  fs.closeSync(fd);
}, 1000);

// https://github.com/joyent/node/issues/2293 - non-persistent watcher should
// not block the event loop
fs.watch(__filename, {persistent: false}, function() {
  assert(0);
});