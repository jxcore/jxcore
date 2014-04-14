# SQLite3

## Credits:

* Author: [Mapbox](https://github.com/mapbox)
* Source on git: [node-sqlite3](https://github.com/mapbox/node-sqlite3)

## Installation

No installation is needed, since this module is embedded inside JXcore.

## Description

Asynchronous, non-blocking SQLite3 bindings for JXcore and Node.js.

Besides SQLite3 module, JXcore also has SQLite 3.8.4.3 engine with FTS3, FTS4 embedded.

# Usage

``` js
var sqlite3 = require('sqlite3').verbose();
var db = new sqlite3.Database(':memory:');

db.serialize(function() {
  db.run("CREATE TABLE lorem (info TEXT)");

  var stmt = db.prepare("INSERT INTO lorem VALUES (?)");
  for (var i = 0; i < 10; i++) {
      stmt.run("Ipsum " + i);
  }
  stmt.finalize();

  db.each("SELECT rowid AS id, info FROM lorem", function(err, row) {
      console.log(row.id + ": " + row.info);
  });
});

db.close();
```

FTS4 example:

```js
var sqlite3 = require('sqlite3').verbose();

var db = new sqlite3.Database(':memory:');

db.serialize(function () {
    db.run("CREATE VIRTUAL TABLE ftsTest USING fts4(content TEXT);");

    var stmt = db.prepare("INSERT INTO ftsTest VALUES (?)");
    for (var i = 1; i < 100; i++) {
        stmt.run("A Ipsum " + i);
    }
    stmt.finalize();
    console.log("virtual fts4 table is written");

    console.log("\nFirst 20 results:\n");
    var str = "SELECT rowid AS id, content FROM ftsTest " +
              "where content MATCH 'Ipsum' LIMIT " + 20;

    db.all(str, function (err, rows) {
        if (err) {
            console.log(err + "");
        }
        else {
            rows.forEach(function (row) {
                console.log(row.id + ": " + row.content);
            });
        }
    });
});
```

# Features

 - Straightforward query and parameter binding interface
 - Full Buffer/Blob support
 - Extensive [debugging support](https://github.com/mapbox/node-sqlite3/wiki/Debugging)
 - [Query serialization](https://github.com/mapbox/node-sqlite3/wiki/Control-Flow) API
 - [Extension support](https://github.com/mapbox/node-sqlite3/wiki/Extensions)
 - Big test suite
 - Written in modern C++ and tested for memory leaks
 - built-in FTS3
 - FTS4 enabled

# Contributors

* [Konstantin Käfer](https://github.com/kkaefer)
* [Dane Springmeyer](https://github.com/springmeyer)
* [Will White](https://github.com/willwhite)
* [Orlando Vazquez](https://github.com/orlandov)
* [Artem Kustikov](https://github.com/artiz)
* [Eric Fredricksen](https://github.com/grumdrig)
* [John Wright](https://github.com/mrjjwright)
* [Ryan Dahl](https://github.com/ry)
* [Tom MacWright](https://github.com/tmcw)
* [Carter Thaxton](https://github.com/carter-thaxton)
* [Audrius Kažukauskas](https://github.com/audriusk)
* [Johannes Schauer](https://github.com/pyneo)
* [Mithgol](https://github.com/Mithgol)


# Acknowledgments

Thanks to [Orlando Vazquez](https://github.com/orlandov),
[Eric Fredricksen](https://github.com/grumdrig) and
[Ryan Dahl](https://github.com/ry) for their SQLite bindings, and to mraleph on Freenode's #v8 for answering questions.

Development of this module is sponsored by [MapBox](http://mapbox.org/).


# License

`node-sqlite3` is [BSD licensed](https://github.com/mapbox/node-sqlite3/raw/master/LICENSE).
