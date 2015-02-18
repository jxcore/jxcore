// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing FTS4 feature of embedded sqlite3 module
 */


var sqlite3 = require('sqlite3').verbose();
var jx = require('jxtools');
var assert = jx.assert;

var finished = false;
var cnt = 0;


var db = new sqlite3.Database(':memory:', null, function (err) {
  assert.strictEqual(err, null, "Cannot create db. " + err);
});

db.serialize(function () {
  //db.run("CREATE TABLE lorem (info TEXT)");
  db.run("CREATE VIRTUAL TABLE ftsTest USING fts4(content TEXT);");

  var stmt = db.prepare("INSERT INTO ftsTest VALUES (?)");
  for (var i = 1; i < 100; i++) {
    stmt.run("A Ipsum " + i);
  }
  stmt.finalize();
  //console.log("virtual fts4 table is written");

  //console.log("\nFirst 20 results:\n");
  db.all("SELECT rowid AS id, content FROM ftsTest where content MATCH 'Ipsum' LIMIT " + 20, function (err, rows) {
    if (err) {
      console.log(err + "");
    }
    else {
      rows.forEach(function (row) {
        //console.log(row.id + ": " + row.content);
        cnt++;
        var str = "A Ipsum " + row.id;
        assert.strictEqual(str, row.content, "row.content should be `" + str + "` but is: `" + row.content + "`");
      });
    }
  });
});


db.close(function (err) {
  assert.strictEqual(err, null, "Cannot close db. " + err);
  finished = true;
  if (process.threadId !== -1)
    process.release();
});


process.on('exit', function (code) {
  assert.ok(finished, "Test did not finish");
  assert.strictEqual(cnt, 20, "Count should be 20.");
});