// Copyright & License details are available under JXCORE_LICENSE file

var sqlite3 = require('sqlite3').verbose();
var fs = require('fs');

var jx = require('jxtools');
var assert = jx.assert;

var dbfile = 'file' + process.threadId;


if (fs.existsSync(dbfile)) {
  fs.unlinkSync(dbfile);
}

var db = new sqlite3.Database(dbfile);
var z = 0;

var done = function() {
  if (fs.existsSync(dbfile))
    fs.unlinkSync(dbfile);
  assert.strictEqual(z, 500);
  if (process.subThread)
    process.release();
};

db.serialize(function () {
  db.run("CREATE TABLE lorem (info TEXT)");

  var stmt = db.prepare("INSERT INTO lorem VALUES (?)");
  for (var i = 0; i < 500; i++) {
    stmt.run("Ipsum " + i);
  }
  stmt.finalize();

  db.each("SELECT rowid AS id, info FROM lorem", function (err, row) {
    //console.log(row.id + ": " + row.info);
    if (z++ > 490) {
      db.close(function () {
        done();
      });
    }
  });
});

db.close();

// if done() was not called already...
setTimeout(done, 10000).unref();