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
// for cloud testing platforms
db.run('PRAGMA synchronous = off');
var z = 0;
var total = 500;
var closed = false;

var done = function() {
  if (fs.existsSync(dbfile))
    fs.unlinkSync(dbfile);
  assert.strictEqual(z, total);
  assert.ok(closed, "Database was not closed.");
  if (process.subThread)
    process.release();
};

db.serialize(function () {
  db.run("CREATE TABLE lorem (info TEXT)");

  var stmt = db.prepare("INSERT INTO lorem VALUES (?)");
  for (var i = 0; i < total; i++) {
    stmt.run("Ipsum " + i);
  }
  stmt.finalize();

  db.each("SELECT rowid AS id, info FROM lorem", function (err, row) {
    var id = row.id;
    if (++z >= total) {
      db.close(function (err) {
        if (!err)
          closed = true;
        else
          console.error("DB close error:", err);
        done();
      });
    }
  });
});


// if done() was not called already...
setTimeout(done, 10000).unref();