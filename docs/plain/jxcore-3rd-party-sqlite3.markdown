# SQLite3

## Credits:

* Author: [Mapbox](https://github.com/mapbox)
* Source on git: [node-sqlite3](https://github.com/mapbox/node-sqlite3)

## Installation

No installation is needed, since this module is embedded inside JXcore.

## Description

Asynchronous, non-blocking [SQLite3](http://sqlite.org/) bindings for JXcore and Node.js.

## Depends

 - JXcore v.10.x or Node.js v0.8.x or v0.10.x

Binaries for most Node versions and platforms are provided by default via [node-pre-gyp](https://github.com/springmeyer/node-pre-gyp).

Also works with [node-webkit](https://github.com/rogerwang/node-webkit) and sqlcipher.

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

# Features

 - Straightforward query and parameter binding interface
 - Full Buffer/Blob support
 - Extensive [debugging support](https://github.com/mapbox/node-sqlite3/wiki/Debugging)
 - [Query serialization](https://github.com/mapbox/node-sqlite3/wiki/Control-Flow) API
 - [Extension support](https://github.com/mapbox/node-sqlite3/wiki/Extensions)
 - Big test suite
 - Written in modern C++ and tested for memory leaks


# API

See the [API documentation](https://github.com/mapbox/node-sqlite3/wiki) in the wiki.

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
